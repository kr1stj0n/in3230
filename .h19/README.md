## Archive dir directory
tar -czvf h2019.tar.gz h2019

## Extract name.tar archive
tar -xzvf h2019.tar.gz

## Encrypt
openssl enc -aes-256-cbc -salt -in h2019.tar.gz -out h2019.tar.gz.enc

## Decrypt
openssl enc -d -aes-256-cbc -salt -in h2019.tar.gz.enc -out h2019.tar.gz
