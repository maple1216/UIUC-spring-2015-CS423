rmmod mp2
make
insmod mp2.ko
./userapp 3 10 2 &
./userapp 5 15 3 &
sleep 1
cat /proc/mp2/status
