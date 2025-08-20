#!/bin/bash
## Prerequisites

### Download and install nvm

curl -o- <https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh> | bash
\. "$HOME/.nvm/nvm.sh"
nvm install 22
node -v # Should print "v22.18.0".
nvm current # Should print "v22.18.0".
npm -v # Should print "10.9.3".

### Install liba2sound package

sudo apt install libasound2-dev

## Install Server Local RNBO

git clone <https://github.com/nuitsdesbassins2025/server-local-rnbo.git>

cd server-local-rnbo

npm init -y

npm install @rnbo/js node-web-audio-api

node dodgeball_server.js