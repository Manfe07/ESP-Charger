from flask import Flask, render_template
import charger_connection

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html', data = charger_connection.get_values())

if __name__ == "__main__":
    app.run(debug=True)