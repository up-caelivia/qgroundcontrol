# Build Docker images

## Prerequisites

### Isntall Docker

Follow th guide: https://docs.docker.com/engine/install/ubuntu/

```bash
# Add Docker's official GPG key:
sudo apt-get update
sudo apt-get install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

# Add the repository to Apt sources:
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update

# Install the packages
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

## Linux 
Run the command on wsl (dot included) from repository folder (qgroundcontrol):

```bash
docker build --file ./deploy/docker/Dockerfile-build-linux -t qgc-linux-docker .  
```

## Android
Run the command on wsl:

```bash
docker build --file ./deploy/docker/Dockerfile-build-android -t qgc-android-docker ./deploy/docker
```

# Run Docker Images to create builds:
Run the command on wsl (dot included) from repository folder (qgroundcontrol):

## Linux 

```bash
docker run -it -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-linux-docker
```

## Android 
replace ??????? with the keystore password

```bash
mkdir -p build-docker 
docker run -it \
  --mount type=bind,source="${PWD}",target=/home/user/qgroundcontrol \
  -e FAST=false \
  -e ANDROID_KEYSTORE_PASSWORD=up-caelivia
  qgc-android-docker
```

# Use github action
To be Added

## install required packages

# Run github action locally to create builds:
Run the command on wsl (dot included) from repository folder (qgroundcontrol):

### Linux 

```bash
ACT=true act -j build -W .github/workflows/linux_release.yml
```
### Android 

```bash
ACT=true act -s ANDROID_KEYSTORE_PASSWORD=?????? -P ubuntu-20.04=catthehacker/ubuntu:act-20.04 -j build -W .github/workflows/android_release.yml
```
