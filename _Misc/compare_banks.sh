#!/bin/bash

DoSome()
{
    gcc bytecmp_$1.c -o byte$1
    ./byte$1 > res$1.txt
}

DoSome dmx
DoSome op3
DoSome tmb
DoSome ibk
DoSome ibk2
DoSome ibk3
DoSome bnk
