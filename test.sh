make
./ntsc-test-data input.pnm > ntsc
gcc -Wall ddd.c -o ddd
./ddd < ntsc > usrp
( while true ; do  cat usrp ; done ) | sudo /usr/local/share/uhd/examples/tx_samples_from_file --freq 0 --rate 4000000 --file /dev/stdin --type float
