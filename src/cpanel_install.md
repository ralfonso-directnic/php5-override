##Install/Compile for cpanel

cd into /home/cpeasyapache/src/{INSTALLED PHP VERSION}/ext/
mkdir override

Copy the files from this directory (src) into override

Run the following

phpize
./configure --enable-override
make 
make install

Then just follow the install instructions for the config and php.ini settings
