if [[ -v THREADS ]]; then
  THREADS="-p ${THREADS:-4}"
fi
OUTPUT_FORMAT="${OUTPUT_FORMAT:-}"

for d in "ldraconic" "lsingly" "ldoubly" "ldoubly_cursor" "lsingly_cursor" "lsingly_cursor_fetch" ; do
  (
    set -x
    # deterministic with k(i) = i
    ./build/$d -B D $THREADS -n 100000 -r 1 -R 1 $OUTPUT_FORMAT

    # deterministic with k(i)=t+ip
    ./build/$d -B D $THREADS -n 10000 $OUTPUT_FORMAT

    # steady/randomized
    ./build/$d -B S $THREADS -f 1000 -U 10000 -S 1000 -c 1000000 $OUTPUT_FORMAT
  )
  echo "------"
done
