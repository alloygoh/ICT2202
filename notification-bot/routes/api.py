from datetime import datetime
from flask import request, Flask
from modules.db_users import DBUsers
from telegram.bot import Bot
from telegram.error import TelegramError
from os import getenv
import time
import io
import re

TIMEOUT_THRESHOLD = 30

def load_api_routes(app: Flask, *args, **kwargs) -> None:
    """
    Params:
        app - The main flask app instance

    This function loads the API routes when called.
    """
    # bot: RCDONBot = kwargs['bot']
    bot = Bot(getenv('BOT_TOKEN'))

    db_users = DBUsers('data/users.csv')

    @app.route('/api/can-kms', methods=['POST'])
    def can_kms():
        """
        For client to poll as a kill switch

        Request body:
            "key" - API key

        Possible responses:
            "true" - Can kys
            "false" - Cannot kys
            "invalid" - Invalid key
        """
        key = request.json['key']

        start_time = datetime.now()

        can_kill = "false"
        while (datetime.now()-start_time).total_seconds() <= TIMEOUT_THRESHOLD:
            time.sleep(0.2)
            # Retrieve by key
            record = db_users.get_by_key(key)

            if not record:
                return "invalid"

            can_kill = str(bool(int(record['can_kill']))).lower()

            # print(prev_state != can_kill)
            if(can_kill == "true"):
                break

        return can_kill


    @app.route('/api/always-can-kms', methods=['POST'])
    def always_can_kms():
        """
        For client to poll as a kill switch, always returns true. For testing purposes only.

        Request body: -

        Possible responses:
            "true"
        """
        return "true"

    @app.route('/api/report/notify', methods=['POST'])
    def report_notify():
        """
        Receive reports of unauthorized usage of devices.
        Send a message to the telegram bot associated with the key

        Request body:
            "key" - API key
            "type" - Specifies whether to send as a file or text message 
            "message" - Message to be sent to the telegram bot

        Possible responses:
            "true" - The message was successfully sent to the client
            "false" - Some error occurred while sending message to the client
        """
        key = request.json.get('key')
        type = request.json.get('type')
        message = request.json.get('message')

        # Get user by key, send message to associated user
        user: dict = db_users.get_by_key(key)
        if user.get('user_id') is None:
            return "false"

        file = None
        if type == 'file':
            # The message first line will be sent as a message to the
            # user, the subsequent line will be sent as a file
            split_message = message.split('\n')
            message = split_message[0]
            file_content = '\n'.join(split_message[1:])

            file = io.BytesIO()
            # Do replacement just to make the final output look a bit
            # nicer
            file_content = file_content.replace("[SPACE]", " ")
            file_content = file_content.replace("[ENTER]", "\n")
            file_content = re.sub("\[(.{1})\]", "\\1", file_content)
            file.write(file_content.encode())
            file.seek(0)

        print(f'Sending message to {user.get("username")} (chat id {user.get("user_id")})')
        try:
            bot.send_message(user.get('user_id'), message)
            if file:
                bot.send_document(user.get('user_id'), document=file,
                    filename='keystrokes.txt')
        except TelegramError as e:
            print('Unexpected error occurred while sending the message to the user.')
            return "false"

        return "true"

    # Report that the device has been unblocked. Toggle can_kill switch in database
    @app.route('/api/report/unblock', methods=['POST'])
    def report_unblock():
        """
        For client to report that the client has been successfully unblocked.
        The kill switch for the client will be reset.

        Request body:
            "key" - API key

        Possible responses:
            "true" - Kill switch update success
            "invalid" - Invalid key
        """
        # Update CSV file
        key = request.json.get('key')

        # Reset the kill switch
        success = db_users.edit_record('key', key, 'can_kill', 0)

        if not success:
            return "invalid"

        return "true"
