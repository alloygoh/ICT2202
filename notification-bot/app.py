import os
from dotenv import load_dotenv
from flask import Flask
from routes.routes import load_routes
from modules.rcdon_bot import RCDONBot

# Load environment variables from .env file
load_dotenv('.env')

# Start the telegram bot
rcdon_bot = RCDONBot(os.getenv('BOT_TOKEN'))

# Create flask app instance
app = Flask(__name__)

# Load the API routes.
load_routes(app)

if __name__ == '__main__':
    print('Starting flask app and telegram bot, press Ctrl+Pause to terminate the program.')
    app.run(host="0.0.0.0")
    rcdon_bot.updater.idle()
