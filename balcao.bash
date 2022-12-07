rm /tmp/BALCAO
clear
export MAXCLIENTES="2"
export MAXMEDICOS="2"
gcc -o balcao balcao.c -pthread Admistrador.c 
./balcao