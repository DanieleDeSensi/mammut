#::amesterdebug::set options 1
#::amesterdebug::set network 1
#::amesterdebug::set ame 1
#::amesterdebug::set sensor 1
#::amesterdebug::set gui 1
#::amesterdebug::set budget 1
#::amesterdebug::set power 1
#::amesterdebug::set scope 1
#::amesterdebug::set ta 1
#::amesterdebug::set bc 1
#::amesterdebug::set health 1
#::amesterdebug::set login 1
#::amesterdebug::set histogram 1

####### CONFIGURATION VARIABLES ########
set ::p8host rephrase
set ::fspaddr 169.254.3.234
set ::fsppass abc123
########################################


################# Please don't modify any of the following ##################
#set ::my_sensor_list "all"
#set ::my_sensor_list {PWR250US}
set ::my_sensor_list {PWR250USP0}
set ::datadir "/tmp/amester"

#Sampling interval (Milliseconds)
set ::samplinginterval 0
set ::fps [dict create]
set ::nodes 1
set ::firstread "true"

# Cleaning old sensors files
eval file delete [glob -nocomplain $::datadir/*]

# Connect to the POWER server
puts "Connecting to $::fspaddr ..."
netc myfsp -addr $::fspaddr -tool MSGP -passwd $::fsppass
puts "myfsp is created."
puts "About to begin tracing..."

if {$::my_sensor_list ne "all"} {
    foreach s $::my_sensor_list {
    set filename "${::datadir}/${s}"
    set tracefp [open $filename "w+"]
        #Set write buffer size to be 500000 bytes so the AmesterPoller will read full lines.
        fconfigure $tracefp -buffersize 500000
        dict set ::fps $s $tracefp
    if {[string match *PWR* $s]} {
        regsub -all "PWR" $s "JLS" s
        set filename "${::datadir}/${s}"
        set tracefp [open $filename "w+"]
        #Set write buffer size to be 500000 bytes so the AmesterPoller will read full lines.
        fconfigure $tracefp -buffersize 500000
        dict set ::fps $s $tracefp
    }
    }
}

# Tell AME object to monitor the desired sensors.
set ::theameclist [myfsp get ameclist]
if {$::theameclist eq ""} {
    puts "ERROR: $::fspaddr does not have TPMD firmware supporting Amester"
    exit_application
}
set ::allsensors {}
set ::createfps "true"
foreach amec $::theameclist {
    if {$::my_sensor_list eq "all"} {
    set allsensorobjs [$amec get sensors]
    set allsensornames {}
    foreach s $allsensorobjs {
        lappend allsensornames [$s cget -sensorname]
        if {$::createfps eq "true"} {
        set name [$s cget -sensorname]
        set filename "${::datadir}/${name}"
        set tracefp [open $filename "w+"]
                #Set write buffer size to be 500000 bytes so the AmesterPoller will read full lines.
                fconfigure $tracefp -buffersize 500000
                dict set ::fps $name $tracefp
        if {[string match *PWR* $name]} {
            regsub -all "PWR" $name "JLS" name
            set filename "${::datadir}/${name}"
            set tracefp [open $filename "w+"]
            #Set write buffer size to be 500000 bytes so the AmesterPoller will read full lines. 
            fconfigure $tracefp -buffersize 500000
            dict set ::fps $name $tracefp
        }
        }
    }
        set ::createfps "false"
    set ::my_sensor_list [lsort -ascii $allsensornames]
    }
    $::amec set_sensor_list $::my_sensor_list
    # Make allsensors, a list of all sensor objects
    set ::allsensors [concat $::allsensors [$::amec get sensorname $::my_sensor_list]]
}

proc timestamp {} {
    return [clock milliseconds]
}

# Set the callback for all sensor data updates
set ::new_data_callback my_data_callback

set ::values [dict create]
set ::timestamps [dict create]

proc my_data_callback {sensorobj} {
    # Only print results after all sensors have updated.  Since
    # sensors are all queued waiting to be updated, we can just wait
    # for a particular one to update and know that all of the others
    # have also been updated.
    #
    # Therefore, if this isn't the very first sensor, skip this update
    if {$sensorobj != [lindex $::allsensors 0]} {return}
    # emit readings
    if {$::firstread eq "false"} {  
        set ::firstsensor "true"
        foreach s $::allsensors {
            set shortName [regsub "^::" $s ""]
            set fields [split $shortName _]

            set sensorCommonName [lindex $fields end]
            set unit [$s cget -u_value]

            # For PWR sensors we do a special management to improve accuracy
            if {[string equal $unit "W"]} {
                set scale [$s cget -scalefactor]
                set value [$s cget -value_acc]
                set value [expr {$value*$scale}]
                set freq [$s cget -freq]
                set joules [expr {$value/$freq}]
                
                #set currentts [$s cget -localtime]
                set currentts [timestamp]
                if {$::firstsensor eq "true"} {
                    dict for {thekey theval} $::fps {
                        seek $theval 0 start
                        puts -nonewline $theval "$currentts,"
                    }
                    set ::firstsensor "false"
                }

                #set sensorName $shortName

                set oldjoules 0
                set interval 0
                if {[dict exists $::values $s]} {
                    set oldjoules [dict get $::values $s]
                    set oldts [dict get $::timestamps $s]
                    set interval [expr {($currentts - $oldts) / 1000.0}]
                }
                set watts [expr {($joules - $oldjoules)/$interval}]
                dict set ::values $s $joules
                dict set ::timestamps $s $currentts


                set tracefp [dict get $::fps $sensorCommonName]
                puts -nonewline  $tracefp "$watts,"

                set value [expr {$value/$freq}]
                set jlsName $sensorCommonName
                regsub -all "PWR" $jlsName "JLS" jlsName
                set tracefp [dict get $::fps $jlsName]
                puts -nonewline  $tracefp "$joules,"
            } else {
                set scale [$s cget -scalefactor]
                set value [$s cget -value]
                set value [expr {$value*$scale}]
                set tracefp [dict get $::fps $sensorCommonName]
                puts -nonewline  $tracefp "$value,"
            }
        }
        dict for {thekey theval} $::fps {
            puts $theval ""
        }
    } else { 
        set ::firstread "false" 
    } 
    dict for {thekey theval} $::fps {
        flush $theval
    }
    #after $::samplinginterval
}
