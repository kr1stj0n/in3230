## Archive dir directory
tar -czvf h2020.tar.gz h2020

## Extract name.tar archive
tar -xzvf h2020.tar.gz

## Encrypt
openssl enc -aes-256-cbc -salt -in h2020.tar.gz -out h2020.tar.gz.enc

## Decrypt
openssl enc -d -aes-256-cbc -salt -in h2020.tar.gz.enc -out h2020.tar.gz
