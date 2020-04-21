#/bin/bash

echo -en "TIME\t\tCPU\tSYS\tHDD\n"
while true; do
        echo -en `date +'%H:%M:%S'` "\t"
        echo -en $((`cat /sys/class/thermal/thermal_zone0/temp` / 1000)) "\t"
        echo -en `mcu_ctl tmp_get_c | awk '{print $2}'` "\t"
        echo -en `smartctl -A /dev/sda | awk '/Temperature_Celsius/{print $10}'` "\n"
        sleep 2
done
