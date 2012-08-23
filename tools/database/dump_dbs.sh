#! /bin/bash

for(( i=0; i < 100; i++)); do
mysqldump -uroot -pta0mee  -d --add-drop-database --database "db_map_$i"
done

#mysqldump -uroot -pta0mee -d --create-options -B "$str db_map_config"
mysqldump -uroot -pta0mee  -d --add-drop-database --database "db_map_config"
