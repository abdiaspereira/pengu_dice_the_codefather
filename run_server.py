# 🟢 run_server.py
# Este archivo se usa solo para iniciar tu app Flask con soporte Socket.IO (Eventlet)
# y asegurar que el serial listener esté activo.

import eventlet
eventlet.monkey_patch()  # 🔧 Debe ir ANTES de cualquier import de Flask o SocketIO

import threading
from app import app, socketio, serial_listener, open_serial, ser

# ============================================================
# 🧩 Inicia el hilo de lectura serial si hay un Arduino conectado
# ============================================================
def iniciar_hilo_serial():
    """Lanza un hilo en segundo plano para escuchar el puerto serial."""
    if ser is None or not ser.is_open:
        print("Intentando abrir puerto serial...")
        open_serial()

    hilo = threading.Thread(target=serial_listener, daemon=True)
    hilo.start()
    print("🧵 Hilo serial iniciado correctamente.")

# ============================================================
# 🚀 Arrancar servidor con Eventlet
# ============================================================
if __name__ == "__main__":
    iniciar_hilo_serial()
    print("🌍 Servidor Flask-SocketIO corriendo en http://127.0.0.1:5000")
    socketio.run(app, host="0.0.0.0", port=5000, debug=False)

