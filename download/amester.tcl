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
set ::p8host address_of_the_power8
set ::fspaddr address_of_the_fsp
set ::fsppass password_of_the_fsp
########################################


################# Please don't modify any of the following ##################
#set ::my_sensor_list "all"
#set ::my_sensor_list {PWR250US}
set ::my_sensor_list {PWR250US}
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

# initial values
foreach s $::allsensors {
    set ::acc_begin($s) [$s cget -value_acc]
    set ::update_begin($s) [$s cget -updates]
    set ::timestamp_begin($s) [$s cget -localtime]
    set ::joules_begin($s) 0
}

proc timestamp {} {
    return [clock milliseconds]
}

# Set the callback for all sensor data updates
set ::new_data_callback my_data_callback

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

    		set value [$s cget -value]
    		set scale [$s cget -scalefactor]
    		# Frequency of update in Hz
    		set freq [$s cget -freq]
    		set ::acc_end($s) [$s cget -value_acc]
    		set ::update_end($s) [$s cget -updates]
    		set ::timestamp_end($s) [$s cget -localtime]
    		#set ::timestamp_end($s) [expr $::timestamp_end($s)/1000.0]
    		#calculate average
    		set updates [expr $::update_end($s) - $::update_begin($s)]
    		set accdiff [expr $::acc_end($s) - $::acc_begin($s)]
            # Interval between two successive samples (milliseconds)
    		set interval [expr $::timestamp_end($s) - $::timestamp_begin($s)] 
    		#correct accdiff for wrap-around of 32-bit accumulator in AME firmware
    		set accdiff_fix [expr (round($accdiff / $scale) & 0x0ffffffff) * $scale]
            if {$::firstsensor eq "true"} {
                dict for {thekey theval} $::fps {
                    seek $theval 0 start
                    puts -nonewline $theval "$::timestamp_end($s),"
                }
                set ::firstsensor "false"
            }

            # For each sample we read, Amester stores N samples.
            # We do the average over these N samples. This is ALWAYS correct, DON'T change
    		if {$updates > 0} {
    		    set avg [expr double($accdiff_fix)/$updates]
    		} else {
    		    # Avoid divide by 0 when sensor does not update in desired interval.
    		    # Print the sensor value as the average
    		    set avg $value
    		}
    		set ::acc_begin($s) $::acc_end($s)
            set ::update_begin($s) $::update_end($s)
            set ::timestamp_begin($s) $::timestamp_end($s)
    		set tracefp [dict get $::fps $sensorCommonName]
            puts -nonewline  $tracefp "$avg,"

    		# For PWR sensors we also create a JLS (joules) sensor
    	    if {[string equal $unit "W"]} {
        		set ::joules_end($s) [expr $::joules_begin($s) + ($avg*($interval/1000.0))]		
        		set ::joules_begin($s) $::joules_end($s)
                set jlsName $sensorCommonName
                regsub -all "PWR" $jlsName "JLS" jlsName
                set tracefp [dict get $::fps $jlsName]
                puts -nonewline  $tracefp "$::joules_end($s),"
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
