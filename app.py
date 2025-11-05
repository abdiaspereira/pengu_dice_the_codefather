import os
from flask import Flask, render_template, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO
import time
import serial
import threading

# --- Flask & DB ---
app = Flask(__name__)
app.config['SECRET_KEY'] = 'devkey'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///scores.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
socketio = SocketIO(app, async_mode='eventlet')

# --- Modelo DB ---
class Score(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(80))
    score = db.Column(db.Integer)
    timestamp = db.Column(db.DateTime, server_default=db.func.now())

# Crear DB si no existe
with app.app_context():
    db.create_all()

# --- Serial ---
SERIAL_PORT = 'COM5'  # ⚙️ Ajusta según tu puerto
SERIAL_BAUD = 115200
ser = None

def open_serial():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
        print("✅ Serial abierto:", SERIAL_PORT)
    except Exception as e:
        ser = None
        print("⚠️ No se pudo abrir el puerto serial:", e)

def serial_listener():
    global ser
    while True:
        if ser and ser.is_open:
            try:
                line = ser.readline().decode(errors='ignore').strip()
                if not line:
                    time.sleep(0.01)
                    continue

                print("Serial<-", line)

                # --- Eventos recibidos ---
                if line.startswith("GAME_OVER:"):
                    score = int(line.split(":")[1])
                    socketio.emit('game_over', {'score': score})

                elif line.startswith("READY"):
                    socketio.emit('status', {'status': 'ready'})

                elif line.startswith("LEVEL_UP"):
                    print("🎉 Nivel superado")
                    socketio.emit('level_up')  # 🔥 Enviamos evento a la web

            except Exception as e:
                print("Error serial:", e)
                time.sleep(1)
        else:
            open_serial()
            time.sleep(2)

# --- Rutas ---
@app.route('/')
def index():
    scores = Score.query.order_by(Score.score.desc(), Score.timestamp).limit(20).all()
    return render_template('index.html', scores=scores)

@app.route('/start', methods=['POST'])
def start_game():
    if ser and ser.is_open:
        try:
            ser.write(b'START\n')
            return jsonify({'status': 'sent'})
        except Exception as e:
            return jsonify({'status': 'error', 'msg': str(e)}), 500
    return jsonify({'status': 'noconnect'}), 500

@app.route('/submit_score', methods=['POST'])
def submit_score():
    data = request.get_json()
    name = data.get('name', 'Anon')
    score_val = int(data.get('score', 0))
    s = Score(name=name, score=score_val)
    db.session.add(s)
    db.session.commit()
    socketio.emit('new_score', {'name': s.name, 'score': s.score})
    return jsonify({'ok': True})

@app.route('/delete_db', methods=['POST'])
def delete_db():
    try:
        num_deleted = Score.query.delete()
        db.session.commit()
        return jsonify({'ok': True, 'msg': f'Se eliminaron {num_deleted} registros de puntaje'})
    except Exception as e:
        db.session.rollback()
        return jsonify({'ok': False, 'msg': f'Error al eliminar datos: {str(e)}'})

# --- SocketIO ---
@socketio.on('connect')
def on_connect():
    print('🌐 Cliente conectado')

# --- Main ---
if __name__ == '__main__':
    threading.Thread(target=serial_listener, daemon=True).start()
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
