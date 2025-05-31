FROM zimbora/arduino-deploy-amd:latest

ENV BASE_DIR=/root
ARG PROJECT="esp32-freeRTOS2"
ARG APP="sniffer-gw"
ARG BUILD="dev"

# Copy the script into the container
COPY . /${PROJECT}

# Make the script executable
RUN chmod +x /${PROJECT}/deploy.sh

# Set the working directory to your project directory
WORKDIR /${PROJECT}

RUN ./deploy.sh -d $BASE_DIR -p $PROJECT -a $APP -b $BUILD
