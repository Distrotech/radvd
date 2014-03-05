
./radvd_test -C check/configs/trivial.conf -m stderr -d 5 -n -p pid.txt &
sleep 0.5
kill $(cat pid.txt)

