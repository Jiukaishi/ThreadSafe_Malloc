echo "===================Making================================================="
make clean
make
for i in {1..20}
	do
echo "====================$i thread_test_measurement==========================="
./thread_test_measurement
done
echo "===================Cleaning==============================================="
make clean
