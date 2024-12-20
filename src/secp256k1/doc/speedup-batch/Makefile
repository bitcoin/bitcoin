schnorrsig_data = schnorrsig_batch.dat schnorrsig_single.dat
tweak_data = tweak_batch.dat tweak_single.dat

bench_output.txt: bench.sh
	SECP256K1_BENCH_ITERS=500000 ./bench.sh bench_output.txt

schnorrsig_batch.dat: bench_output.txt
	cat bench_output.txt | grep -v "schnorrsig_batch_verify_1 " | awk '{ gsub(/ /,""); print }' | awk -F, 'match($$0, /schnorrsig_batch_verify_([0-9]+)/, arr) {print arr[1] " " $$3}' > schnorrsig_batch.dat

schnorrsig_single.dat: bench_output.txt
	cat bench_output.txt | awk '{ gsub(/ /,""); print }' | awk -F, 'match($$0, /schnorrsig_verify/) {print $$3}' > schnorrsig_single.dat

tweak_batch.dat: bench_output.txt
	cat bench_output.txt | grep -v "tweak_check_batch_verify_1 " | awk '{ gsub(/ /,""); print }' | awk -F, 'match($$0, /tweak_check_batch_verify_([0-9]+)/, arr) {print arr[1] " " $$3}' > tweak_batch.dat

tweak_single.dat: bench_output.txt
	cat bench_output.txt | awk '{ gsub(/ /,""); print }' | awk -F, 'match($$0, /tweak_add_check/) {print $$3}' > tweak_single.dat

speedup-batch.png: $(schnorrsig_data) $(tweak_data) plot.gp
	gnuplot plot.gp

clean:
	rm *.log *.txt *.dat *.png
