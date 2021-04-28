#!/bin/bash

executable_path=$1
problem_number=$2
user_id=$3
points=0

for (( i=1;i<=5;i++ ))
do
  result=$($executable_path < /home/vlad/CLionProjects/Server/Tests/Input/p${problem_number}_${i}.in)
  test_case=$(cat /home/vlad/CLionProjects/Server/Tests/Output/p${problem_number}_${i}.out)

  if [ $result == $test_case ]
  then
    (( points++ ))
  fi

done
echo $points > /home/vlad/CLionProjects/Server/Results/${user_id}_p${problem_number}.txt
