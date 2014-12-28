for i in {1..35}
do
    value=$(printf "%03d" $i)
    angle=$(($i * 10))
    convert spinner-000.png -background "rgba(0,0,0,0.0)" -distort SRT $angle spinner-$value.png
done
