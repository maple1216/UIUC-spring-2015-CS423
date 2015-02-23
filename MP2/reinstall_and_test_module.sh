rmmod mp2
make
insmod mp2.ko
echo "R, ddd" > /proc/mp2/status
dmesg | tail -n 20
make clean
