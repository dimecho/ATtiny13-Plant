#!/usr/bin/env bash

main()
{
    killall php
    php -S 127.0.0.1:8080 -t "$(dirname "$0")/Web/" &
    /usr/bin/firefox http://127.0.0.1:8080;bash
}

if [[ $(type -p php) ]]; then
    printf "PHP \e[92m[OK]\e[0m\n"
else
    printf "PHP \e[91mNot Installed\e[0m ...Install? [Y/n]\n"; read
    sudo apt-get update
	sudo apt-get -y install php
fi

if [[ $(type -p avrdude) ]]; then
	printf "AVRDUDE \e[92m[OK]\e[0m\n"
    main
else
	printf "AVRDUDE \e[91mNot Installed\e[0m ...Install? [Y/n]\n"; read
	sudo apt-get update
	sudo apt-get -y install avrdude
    main
fi