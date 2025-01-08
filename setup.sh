#!/bin/bash

# Ensure the script is executed with root privileges
if [ "$EUID" -ne 0 ]; then
  echo "This script must be run as root. Please use 'sudo' to execute it."
  exec sudo "$0" "$@"
fi


# Define variables
HOME_DIR=$(eval echo "~$SUDO_USER")
DIR="$HOME_DIR/.bluetracker"
FILE="bluetracker"
PATH_C=$(dirname "$(realpath "$0")")
OK='[  \033[0;32mOK\033[0m  ] '                  # Visual indicator for successful operations
ERROR='[ \033[0;31mERROR\033[0m ]'               # Visual indicator for errors
WARNING='[\033[38;5;214mWARNING\033[0m]'         # Visual indicator for warnings


# Check if the ble.c file exists
if [[ ! -e "$PATH_C/ble.c" ]]; then
    printf "${ERROR} The 'ble.c' file was not found in the current directory ($PATH_C)\n"
    exit 1
fi


# Install required dependencies
printf "Installing required dependencies..."
if ! sudo apt install -y build-essential libbluetooth-dev > /dev/null 2>&1; then
    printf "\r${ERROR} Failed to install required dependencies\n"
    exit 1
fi
printf "\r${OK} Required dependencies installed\n"


# Create the initialization script
printf "Creating wrapper file..."
cat <<EOF > $FILE
#!/bin/bash
if [ "\$EUID" -ne 0 ]; then
  exec sudo "\$0" "\$@"
fi
HOME_DIR=\$(eval echo "~\$SUDO_USER")
sudo \$HOME_DIR/.bluetracker/bluetracker.out "\$@"
EOF
printf "\r${OK} Wrapper file created\n"


# Move the script to /usr/bin
printf "Moving wrapper file to /usr/bin/"
sudo mv "$FILE" /usr/bin/
sudo chmod +x /usr/bin/$FILE
printf "\r${OK} Wrapper file moved to /usr/bin/\n"


# Create the program directory
printf "Creating directory $DIR..."
mkdir -p "$DIR"


# Compile the code
if ! gcc "$PATH_C/ble.c" -o "$DIR/bluetracker.out" -lbluetooth -lm > /dev/null 2>&1; then
    echo "\r${ERROR} Error during code compilation.\n"
    exit 1
fi
printf "\r${OK} Directory $DIR created"


# Copy the LICENSE file if it exists
if [[ -f "$PATH_C/LICENSE" ]]; then
    cp "$PATH_C/LICENSE" "$DIR/"
else
    printf "\n${WARNING} The LICENSE file was not found.\n"
fi


echo -e "\033[0;32mINSTALLATION COMPLETED\033[0m"
