# Remote Covert Defensive Operations Notification centre (RCDON)
A telegram bot for user to receive notifications on security breaches on their devices.

## Setting up telegram bot
https://core.telegram.org/bots#6-botfather

## Configuring .env
Make a copy of [`env.example`](env.example), naming it `.env`. Fill in `BOT_TOKEN` with the api key obtained from botfather.

## Populating user data
Populate user data in [`data/users.csv`](data/users.csv) file. **Remember to put a trailing comma after `can_kill` field.**
- `username`: Telegram username (only required during intial authentication)
- `key`: Any string, just make sure to tally with the `RCDO_KEY` variable set in the RCDOB (Binary).
- `can_kill`: Default to zero
- `user_id`: Leave blank, the bot will populate this itself

## Python environment
(Optional) Set up virtual environment
```powershell
$ python -m venv venv
$ .\venv\Scripts\activate
$ deactivate #to deactivate
```

Install dependencies
```powershell
$ python -m pip install -r requirements.txt
```

## Usage
```
python app.py
```
