#!/bin/bash

increment_version() {
  local delimiter=.
  local array=($(cat version.txt | tr $delimiter '\n'))
    echo $2
  for index in ${!array[@]}; do
    if [[ $index -eq $1 ]]; then
      local value=array[$index]
      value=$((value+1))
      array[$index]=$value
      break
    fi
  done

  echo $(IFS=$delimiter ; echo "${array[*]}") > version.txt
}

increment_version $1
git add version.txt