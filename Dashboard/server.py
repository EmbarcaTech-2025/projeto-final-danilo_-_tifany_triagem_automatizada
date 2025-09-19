from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime
from collections import deque

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

CABINES = [f"Cabine {i}" for i in range(1, 7)]
pacientes_recentes = deque(maxlen=50)
ultimo_de_cada_cabine = {c: None for c in CABINES}
_rr = 0

def atribuir_cabine(payload, remote_addr):
    global _rr
    if payload.get("cabine"):
        return payload["cabine"]
    c = CABINES[_rr % len(CABINES)]
    _rr += 1
    return c

@app.route("/")
def index():
    return render_template("index.html",
                           cabines=CABINES,
                           ultimo=ultimo_de_cada_cabine,
                           recentes=list(pacientes_recentes))

@app.route("/ping", methods=["GET"])
def ping():
    return jsonify({"status": "ok"}), 200

@app.route("/api/paciente", methods=["POST"], strict_slashes=False)
def api_paciente():
    try:
        data = request.get_json(force=True)
    except Exception as e:
        print("[API] JSON inválido:", e)
        return jsonify({"status":"erro","msg":"JSON inválido"}), 400

    # conversões defensivas
    try:
        data["bpm"] = int(data.get("bpm", 0))
        data["spo2"] = int(data.get("spo2", 0))
        data["temperatura"] = float(data.get("temperatura", 0))
        data["pressao_sys"] = int(float(data.get("pressao_sys", 0)))
        data["pressao_dia"] = int(float(data.get("pressao_dia", 0)))
        data["distancia"] = int(data.get("distancia", 0))
    except Exception as e:
        print("[API] tipos inválidos:", e)
        return jsonify({"status":"erro","msg":"Campos numéricos inválidos"}), 400

    data["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    data["ip_origem"] = request.remote_addr or "desconhecido"
    data["cabine"] = data.get("cabine") or atribuir_cabine(data, data["ip_origem"])

    pacientes_recentes.appendleft(data)
    ultimo_de_cada_cabine[data["cabine"]] = data

    try:
        socketio.emit("novo_paciente", data)
    except Exception as e:
        print("[WS] emit erro:", e)

    print(f"[API] ({data['cabine']}) {data['nome']} {data['bpm']}bpm {data['spo2']}% {data['temperatura']}°C")
    return jsonify({"status":"ok"}), 200

if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000)
