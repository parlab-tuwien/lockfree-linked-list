rm steady_results.csv
for d in "ldraconic" "lsingly" "ldoubly" "ldoubly_cursor" "lsingly_cursor" "lsingly_cursor_fetch" ; do
  for thread in 1 2 4 6 8 12 16 ; do
    for i in {1..5} ; do
      echo "$d $thread threads; run $i"
      ./build/$d -p $thread -B S -f 16384 -U 32768 -A 25 -R 25 -c 50000 -C >> steady_results.csv
    done
  done
done
