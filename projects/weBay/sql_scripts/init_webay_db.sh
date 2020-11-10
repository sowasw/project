#! /bin/bash

conf="../weBayServer.conf"

user=`gawk 'BEGIN{FS="="};/user/ {print $2;}' $conf`
password=`gawk 'BEGIN{FS="="};/password/ {print $2;}' $conf`
db=`gawk 'BEGIN{FS="="};/db_name/ {print $2;}' $conf`

mysql -e "source create_database.sql"
mysql -e "source init_webay_tables.sql" $db

#mysql -u $user -p$password -e "source create_database.sql"
#mysql -u $user -p$password -e "source init_webay_tables.sql" $db
