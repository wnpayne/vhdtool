#!/bin/bash

ibase=$1
obase=$2
toconv=$3

echo "obase=$obase;ibase=$ibase;$toconv" | bc
