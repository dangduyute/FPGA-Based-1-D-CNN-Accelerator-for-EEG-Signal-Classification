echo 0 > /sys/class/fpga_manager/fpga0/flags
cp ip_wrapper.bit /lib/firmware
echo ip_wrapper.bit >/sys/class/fpga_manager/fpga0/firmware