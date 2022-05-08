for i in {1..10}
do
    echo "--------------------------------------------"
    echo "              Run number $i"
    echo "--------------------------------------------"
    ./countWords dataset/text0.txt dataset/text1.txt dataset/text2.txt dataset/text3.txt dataset/text4.txt   
done