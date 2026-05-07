FROM openfoam/openfoam10-default:latest

LABEL description="OpenFOAM Learning Assistant – Claude Code Agent"

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    nodejs \
    npm \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN npm install -g @anthropic-ai/claude-code

WORKDIR /workspace

COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

COPY . /workspace/openfoam_agents

ENV WM_PROJECT_DIR=/opt/openfoam10
ENV PYTHONPATH=/workspace/openfoam_agents

RUN echo "source /opt/openfoam10/etc/bashrc" >> /root/.bashrc

EXPOSE 8080

CMD ["/bin/bash"]
