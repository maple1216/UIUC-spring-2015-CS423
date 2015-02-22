rmmod mp2
make
insmod mp2.ko
#./userapp 10 &
#./userapp 15 &
#sleep 6
head -n 10 /proc/mp2/status
make clean
