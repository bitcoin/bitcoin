FRAMEDIR=$(dirname $0)
for i in {0..35}
do
    frame=$(printf "%03d" $i)
    angle=$(($i * 10))
    convert $FRAMEDIR/../src/spinner.png -background "rgba(0,0,0,0.0)" -distort SRT $angle $FRAMEDIR/spinner-$frame.png
done
