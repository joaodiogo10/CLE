for i in {1..1}
do
   mpiexec -n $1 ./main dataset/text0.txt dataset/text1.txt dataset/text2.txt dataset/text3.txt dataset/text4.txt   
done
