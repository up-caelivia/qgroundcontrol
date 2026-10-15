# syntax=docker/dockerfile:1

FROM ubuntu:20.04 AS qgc-build

ARG DEBIAN_FRONTEND=noninteractive
ARG QT_VERSION=5.15.2
ARG QML_TOOLING_QT_VERSION=6.8.0

ENV TZ=Etc/UTC
ENV QT_VERSION=${QT_VERSION}
ENV QT_PATH=/opt/Qt
ENV QT_DESKTOP=/opt/Qt/${QT_VERSION}/gcc_64
ENV QMAKESPEC=linux-g++-64
ENV PATH=/usr/lib/ccache:${QT_DESKTOP}/bin:${PATH}

# Keep this list aligned with the supported Linux build image in deploy/docker,
# plus the tools needed for interactive GUI development and debugging.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        bear \
        binutils \
        build-essential \
        ca-certificates \
        ccache \
        clangd \
        clang-format \
        cmake \
        curl \
        dbus-x11 \
        espeak \
        fluxbox \
        gdb \
        git \
        gosu \
        libdbus-1-3 \
        libegl1 \
        libespeak-dev \
        libfontconfig1 \
        libfuse2 \
        libgl1-mesa-dev \
        libgl1-mesa-dri \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer1.0-0 \
        libgstreamer1.0-dev \
        libssl-dev \
        libsdl2-dev \
        libspeechd-dev \
        libudev-dev \
        libunwind-dev \
        libx11-xcb1 \
        libxcb-cursor0 \
        libxcb-icccm4 \
        libxcb-image0 \
        libxcb-keysyms1 \
        libxcb-randr0 \
        libxcb-render-util0 \
        libxcb-shape0 \
        libxcb-shm0 \
        libxcb-sync1 \
        libxcb-util1 \
        libxcb-xfixes0 \
        libxcb-xinerama0 \
        libxcb-xkb1 \
        libxkbcommon-x11-0 \
        locales \
        make \
        mesa-utils \
        ninja-build \
        novnc \
        patchelf \
        pkg-config \
        procps \
        python3 \
        python3-dev \
        python3-pip \
        rsync \
        speech-dispatcher \
        websockify \
        x11-utils \
        x11vnc \
        xvfb \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# QGC needs Qt private headers, so distribution Qt packages are insufficient.
RUN python3 -m pip install --no-cache-dir "aqtinstall==3.1.18" \
    && aqt install-qt linux desktop "${QT_VERSION}" gcc_64 \
        -O "${QT_PATH}" \
        -m qtcharts \
    && aqt install-qt linux desktop "${QML_TOOLING_QT_VERSION}" linux_gcc_64 \
        -O /opt/QtTools \
        -m qtlanguageserver \
    && rm -rf /root/.cache

RUN locale-gen en_US.UTF-8

FROM qgc-build AS qgc-gui

ARG USERNAME=qgc
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid "${USER_GID}" "${USERNAME}" \
    && useradd --uid "${USER_UID}" --gid "${USER_GID}" --create-home --shell /bin/bash "${USERNAME}" \
    && mkdir -p /workspace-build /ccache /tmp/runtime-qgc \
    && chown "${USERNAME}:${USERNAME}" /workspace-build /ccache /tmp/runtime-qgc

COPY development/docker/entrypoint.sh /usr/local/bin/qgc-entrypoint.sh
COPY development/docker/build-qgc.sh /usr/local/bin/qgc-build.sh
COPY development/docker/run-qgc.sh /usr/local/bin/qgc-run.sh

RUN chmod +x /usr/local/bin/qgc-entrypoint.sh \
    /usr/local/bin/qgc-build.sh \
    /usr/local/bin/qgc-run.sh

ENV DISPLAY=:0
ENV XDG_RUNTIME_DIR=/tmp/runtime-qgc
ENV QT_QPA_PLATFORM=xcb
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV QGC_SOURCE_DIR=/workspaces/qgroundcontrol
ENV QGC_BUILD_DIR=/workspace-build
ENV CCACHE_DIR=/ccache
ENV CCACHE_COMPILERCHECK=content

WORKDIR /workspaces/qgroundcontrol
EXPOSE 6080

ENTRYPOINT ["/usr/local/bin/qgc-entrypoint.sh"]
CMD ["sleep", "infinity"]
