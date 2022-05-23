for i in {1..40}
do
   echo "--------------------------------------------"
   echo "              Run number $i"
   echo "--------------------------------------------"
   mpiexec -n $1 ./main dataset/text0.txt dataset/text1.txt dataset/text2.txt dataset/text3.txt dataset/text4.txt
done
