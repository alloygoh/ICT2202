# USBubble

## C2 Server
A telegram bot for user to receive notifications on security breaches on their devices.

Before reading this section, please change directory to `notification-bot`

### Setting up telegram bot

https://core.telegram.org/bots#6-botfather

### Configuring .env

Make a copy of [`env.example`](env.example), naming it `.env`. Fill in `BOT_TOKEN` with the api key obtained from botfather.

### Populating user data

Populate user data in [`notification-bot/data/users.csv`](notification-bot/data/users.csv) file. **Remember to put a trailing comma after `can_kill` field.**

- `username`: Telegram username (only required during intial authentication)
- `key`: Any string, just make sure to tally with the `USBUBBLE_API_KEY` variable set in the C program.
- `can_kill`: Default to zero
- `user_id`: Leave blank, the bot will populate this itself

### Python environment

(Optional) Set up virtual environment

```powershell
python -m venv .venv
.\.venv\Scripts\activate
deactivate #to deactivate
```

Install dependencies

```powershell
python -m pip install -r requirements.txt
```

### Usage

```
python app.py
```

## Machine learning model
Before reading this section, please change directory to `ml_model`

(Optional) Set up virtual environment

```powershell
python -m venv .venv
.\.venv\Scripts\activate
deactivate #to deactivate
```

Install dependencies

```powershell
python -m pip install -r requirements.txt
```

### Get files for the ML model
https://drive.google.com/drive/folders/1Lv5Q9ctxWs8T8KvwKcQpiGDVAzx34tl2?usp=sharing

Put the files in the `ml_model` folder

### Usage

```
uvicorn main:app
```


## Main C program
### Set the environment variable
Powershell
```powershell
$Env:USBUBBLE_API_KEY = "<insert key here>"
```

### Enrolling new devices
```powershell
./USBubble enroll
```

### Using the program
```powershell
./USBubble
```
