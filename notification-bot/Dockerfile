FROM ubuntu:latest

RUN apt update -y && apt install -y python3 python3-pip python3-dev

COPY ./requirements.txt /app/requirements.txt

WORKDIR /app

RUN pip install -r requirements.txt

COPY . /app

ENTRYPOINT ["python3"]
CMD ["app.py"]
