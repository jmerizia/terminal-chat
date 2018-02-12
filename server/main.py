from flask import Flask
from flask import request
from time import gmtime, strftime

app = Flask(__name__)

messages = []
with open("messages.txt", "r") as messages_file:
    for line in messages_file:
        messages.append(line)
    messages = messages[-20:]

print messages

users = []
with open("users.txt", "r") as users_file:
    for line in users_file:
        users.append(line[:-1])

@app.route("/")
def main():
    return "ok"

@app.route("/get_messages")
def get_messages():
    resp = ""
    for message in messages:
        resp += message
    return resp


@app.route("/new_message", methods=['GET'])
def new_message():
    username = request.args.get('username', '')
    message = request.args.get('message', '')
    if username in users:
        tm = strftime("%Y-%m-%d %H:%M", gmtime())
        prefix = "[" + tm + "] " + username
        with open("messages.txt", "a") as fl:
            message_character_codes = ""
            for letter in message:
                message_character_codes += str(ord(letter)) + " "
            final_msg = prefix + "|" + message_character_codes + '\n';
            fl.write(final_msg)
            messages.append(final_msg)
            if (len(messages) == 21):
                messages.pop(0)
        return "ok"
    else:
        return "bad username"


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
