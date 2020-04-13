#!/usr/bin/env bash

main()
{
    killall php
    if [[ -f "$(dirname "$0")/Web/ip.txt" ]]; then
        php -S 0.0.0.0:8080 -t "$(dirname "$0")/Web/" &
    else
        php -S 127.0.0.1:8080 -t "$(dirname "$0")/Web/" &
    fi
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

    #Install AVRDUDE 5.11.1
    sudo apt-get -y install libftdi1
	wget http://archive.ubuntu.com/ubuntu/pool/main/r/readline6/libreadline6_6.3-4ubuntu2_amd64.deb -P /tmp
    sudo dpkg -i /tmp/libreadline6_6.3-4ubuntu2_amd64.deb
    wget http://archive.ubuntu.com/ubuntu/pool/universe/a/avrdude/avrdude_5.11.1-1_amd64.deb -P /tmp
    sudo dpkg -i /tmp/avrdude_5.11.1-1_amd64.deb

    #AVRDUDE 6.3 requires a patch (apt-get distro will not work)
	#sudo apt-get -y install avrdude
    main
fi