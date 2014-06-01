
./radvd_test -C check/configs/trivial.d -m stderr -d 5 -n -p pid.txt &
sleep 0.5
kill $(cat pid.txt)

