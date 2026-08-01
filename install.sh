#!/bin/bash


echo "Install mdwkit"
echo "These files will be created:"
echo "/opt/mdwkit/"
echo "link: /usr/local/bin/mdwkit"
echo ""

echo -n "Continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


mkdir -p "/opt/mdwkit"
mkdir -p "/opt/mdwkit/plugins"
cp -i bin/mdwkit "/opt/mdwkit/"
cp -i ext/lib/mdwkitext.so "/opt/mdwkit/"
cp -R ./plugins/bin/* /opt/mdwkit/plugins/
chmod 755 /opt/mdwkit/mdwkit
ln -s /opt/mdwkit/mdwkit /usr/local/bin/mdwkit


echo "done"
#read -p "Press [Enter] key to exit..."

else
echo "canceled"
fi


exit 0
