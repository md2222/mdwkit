#!/bin/bash


echo "Uninstall mdwkit"
echo "These files will be deleted:"
echo "/opt/mdwkit/"
echo "link: /usr/local/bin/mdwkit"
echo ""

echo -n "Continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


rm "/usr/local/bin/mdwkit"
rm -r "/opt/mdwkit"


echo "done"
#read -p "Press [Enter] key to exit..."

else

echo "canceled"

fi


exit 0
