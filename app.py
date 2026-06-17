from flask import Flask, request, jsonify

app = Flask(__name__)

state = {
    "message": "No message yet",
    "unread": False
}

@app.route("/set", methods=["POST"])
def set_message():
    state["message"] = request.form.get("msg", "")
    state["unread"] = True
    return "OK"

@app.route("/get")
def get_message():
    return jsonify(state)

@app.route("/ack", methods=["POST"])
def ack():
    state["unread"] = False
    return "OK"

app.run(host="0.0.0.0", port=80)

# this is very very unsecure and uhhhh ill fix it one day
