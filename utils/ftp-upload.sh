#!/bin/bash

UPLOAD_LIST="set ssl:verify-certificate no; put -O $1 $2;"
lftp -e "${UPLOAD_LIST} exit" -u ${FTPUser},${FTPPassword} ${FTPServer}

