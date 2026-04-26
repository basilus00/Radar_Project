import argparse
import threading
import time
import math
import random
import re
from collections import deque
from datetime import datetime

import serial
import plotly.graph_objects as go
import dash
from dash import dcc, html, Input, Output
import dash_bootstrap_components as dbc

# -----------------------------
# Shared state (thread-safe enough for this simple case)
# -----------------------------
MAX_POINTS = 180
lock = threading.Lock()

latest = {
    "angle": 0,
    "distance": 400,
    "connected": False,
    "port": "",
    "baud": 9600,
    "samples": 0,
    "last_ts": time.time(),
}

# Keep a fading history of detections
detections = deque(maxlen=MAX_POINTS)  # items: (x, y, alpha)

# -----------------------------
# Serial parsing
# -----------------------------
LINE_RE = re.compile(r"^\s*(\d+)\s*,\s*(\d+)\s*$")

def ingest(angle: int, distance: int, max_cm: int = 40):
    # Convert to radar coordinates (servo degrees: 0..180)
    # Plot coords: x=cos, y=sin, only y>=0
    a = math.radians(angle)
    with lock:
        latest["angle"] = angle
        latest["distance"] = distance
        latest["connected"] = True
        latest["samples"] += 1
        latest["last_ts"] = time.time()

        if 0 < distance <= max_cm:
            r = distance / max_cm
            x = r * math.cos(a)
            y = r * math.sin(a)
            detections.append([x, y, 255])

def serial_thread(port: str, baud: int):
    while True:
        try:
            with serial.Serial(port, baud, timeout=2) as ser:
                with lock:
                    latest["port"] = port
                    latest["baud"] = baud
                    latest["connected"] = True
                ser.reset_input_buffer()

                while True:
                    line = ser.readline().decode(errors="ignore").strip()
                    if not line:
                        continue
                    m = LINE_RE.match(line)
                    if not m:
                        continue
                    angle = int(m.group(1))
                    dist = int(m.group(2))
                    if 0 <= angle <= 200 and 0 <= dist <= 500:
                        ingest(angle, dist)
        except Exception:
            with lock:
                latest["connected"] = False
            time.sleep(2)

def demo_thread():
    angle = 15
    step = 1
    while True:
        angle += step
        if angle >= 190:
            angle = 190
            step = -1
        elif angle <= 15:
            angle = 15
            step = 1

        # Fake object + noise
        base = 22 + 10 * math.sin(math.radians(angle * 2.2))
        dist = int(max(2, min(60, base + random.uniform(-2.5, 2.5))))

        # occasional out-of-range
        if random.random() < 0.07:
            dist = 400

        ingest(angle, dist)
        time.sleep(0.015)  # similar to servo step delay

# -----------------------------
# Radar figure
# -----------------------------
def make_radar_figure(max_cm: int = 40) -> go.Figure:
    with lock:
        angle = latest["angle"]
        distance = latest["distance"]
        pts = list(detections)

    # fade points
    for p in pts:
        p[2] -= 12
    pts = [p for p in pts if p[2] > 0]
    with lock:
        detections.clear()
        detections.extend(pts)

    a = math.radians(angle)
    sx, sy = math.cos(a), math.sin(a)

    fig = go.Figure()

    # arcs
    for cm in (10, 20, 30, 40):
        r = cm / max_cm
        xs, ys = [], []
        for i in range(181):
            t = math.pi * (i / 180.0)
            xs.append(r * math.cos(t))
            ys.append(r * math.sin(t))
        fig.add_trace(go.Scatter(
            x=xs, y=ys, mode="lines",
            line=dict(color="rgba(80,255,120,0.35)", width=2),
            hoverinfo="skip",
            showlegend=False
        ))

    # angle lines
    for deg in (0, 30, 60, 90, 120, 150, 180):
        t = math.radians(deg)
        fig.add_trace(go.Scatter(
            x=[0, math.cos(t)],
            y=[0, math.sin(t)],
            mode="lines",
            line=dict(color="rgba(80,255,120,0.25)", width=2),
            hoverinfo="skip",
            showlegend=False
        ))

    # baseline
    fig.add_trace(go.Scatter(
        x=[-1, 1], y=[0, 0], mode="lines",
        line=dict(color="rgba(80,255,120,0.35)", width=3),
        hoverinfo="skip",
        showlegend=False
    ))

    # sweep line
    fig.add_trace(go.Scatter(
        x=[0, sx], y=[0, sy], mode="lines",
        line=dict(color="rgba(60,255,120,0.95)", width=6),
        hoverinfo="skip",
        showlegend=False
    ))

    # detected points
    if pts:
        fig.add_trace(go.Scatter(
            x=[p[0] for p in pts],
            y=[p[1] for p in pts],
            mode="markers",
            marker=dict(
                size=10,
                color=[f"rgba(255,50,50,{p[2]/255:.3f})" for p in pts],
                line=dict(width=0)
            ),
            hoverinfo="skip",
            showlegend=False
        ))

    # HUD text (angle + distance)
    fig.add_annotation(
        x=0.0, y=-0.08, xref="paper", yref="paper",
        text=f"<b>Angle:</b> {angle}° &nbsp;&nbsp; <b>Distance:</b> {distance} cm",
        showarrow=False,
        font=dict(color="#7CFF9A", size=16),
    )

    fig.update_layout(
        template=None,
        paper_bgcolor="#06101d",
        plot_bgcolor="#06101d",
        margin=dict(l=10, r=10, t=10, b=10),
        xaxis=dict(visible=False, range=[-1.05, 1.05]),
        yaxis=dict(visible=False, range=[-0.10, 1.05], scaleanchor="x", scaleratio=1),
        height=650
    )
    return fig

# -----------------------------
# Dash App
# -----------------------------
app = dash.Dash(__name__, external_stylesheets=[dbc.themes.DARKLY], title="Radar Dashboard")

app.layout = dbc.Container(fluid=True, children=[
    dbc.Row([
        dbc.Col(html.H3("Ultrasonic Radar Dashboard", style={"marginTop": "10px"}), md=8),
        dbc.Col(html.Div(id="conn", style={"marginTop": "14px", "textAlign": "right"}), md=4),
    ]),
    dbc.Row([
        dbc.Col(dcc.Graph(id="radar", config={"displayModeBar": False}), md=8),
        dbc.Col([
            dbc.Card(dbc.CardBody([
                html.H5("Live Info"),
                html.Div(id="kpis"),
                html.Hr(),
                html.Div("Tip: Use --demo to run without hardware.", style={"opacity": 0.8}),
            ]))
        ], md=4)
    ]),
    dcc.Interval(id="tick", interval=50, n_intervals=0),  # smooth sweep
])

@app.callback(
    Output("radar", "figure"),
    Output("kpis", "children"),
    Output("conn", "children"),
    Input("tick", "n_intervals")
)
def update(_):
    fig = make_radar_figure(max_cm=40)

    with lock:
        a = latest["angle"]
        d = latest["distance"]
        ok = latest["connected"]
        port = latest["port"]
        baud = latest["baud"]
        samples = latest["samples"]

    inrange = (0 < d <= 40)
    kpis = html.Ul([
        html.Li(f"Angle: {a}°"),
        html.Li(f"Distance: {d} cm"),
        html.Li(f"In range (<=40cm): {'YES' if inrange else 'NO'}"),
        html.Li(f"Samples: {samples}"),
    ])

    badge = html.Span(
        "● CONNECTED" if ok else "● DISCONNECTED",
        style={"color": "#39d353" if ok else "#f85149", "fontWeight": "bold"}
    )
    detail = html.Div(f"{port} @ {baud}" if port else "", style={"opacity": 0.8, "fontSize": "12px"})
    conn = html.Div([badge, html.Br(), detail])

    return fig, kpis, conn

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=None, help="COM port, e.g. COM6 or /dev/ttyUSB0")
    parser.add_argument("--baud", default=9600, type=int)
    parser.add_argument("--demo", action="store_true")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--web-port", default=8050, type=int)
    args = parser.parse_args()

    if args.demo or not args.port:
        t = threading.Thread(target=demo_thread, daemon=True)
    else:
        t = threading.Thread(target=serial_thread, args=(args.port, args.baud), daemon=True)
    t.start()

    print(f"Open: http://{args.host}:{args.web_port}/")
    app.run(host=args.host, port=args.web_port, debug=False)

if __name__ == "__main__":
    main()