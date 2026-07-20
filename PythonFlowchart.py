#!/usr/bin/env python3
parts = []
W,H = 1150, 1160

def esc(s):
    return s.replace('&','&amp;').replace('<','&lt;').replace('>','&gt;')

def rect(x,y,w,h,stroke=1.5):
    parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="#ffffff" stroke="#000000" stroke-width="{stroke}"/>')

def pill(x,y,w,h,stroke=1.5):
    r=h/2
    parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" ry="{r}" fill="#ffffff" stroke="#000000" stroke-width="{stroke}"/>')

def diamond(cx,cy,w,h,stroke=1.5):
    pts = f"{cx},{cy-h/2} {cx+w/2},{cy} {cx},{cy+h/2} {cx-w/2},{cy}"
    parts.append(f'<polygon points="{pts}" fill="#ffffff" stroke="#000000" stroke-width="{stroke}"/>')

def label(cx,cy,lines,size=13):
    if isinstance(lines,str):
        lines=[lines]
    n=len(lines)
    start_y = cy - (n-1)*(size+3)/2
    tspans = "".join(f'<tspan x="{cx}" y="{start_y+i*(size+3)}">{esc(l)}</tspan>' for i,l in enumerate(lines))
    parts.append(f'<text font-family="Consolas, monospace" font-size="{size}" fill="#000000" text-anchor="middle">{tspans}</text>')

def arrow(x1,y1,x2,y2):
    parts.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#000000" stroke-width="1.5" marker-end="url(#ah)"/>')

def path(pts):
    d = "M " + " L ".join(f"{x} {y}" for x,y in pts)
    parts.append(f'<path d="{d}" fill="none" stroke="#000000" stroke-width="1.5" marker-end="url(#ah)"/>')

def edge_label(x,y,text_,size=12,anchor="middle"):
    parts.append(f'<text x="{x}" y="{y}" font-family="Consolas, monospace" font-size="{size}" fill="#000000" text-anchor="{anchor}">{esc(text_)}</text>')
    # white halo behind label so crossing lines never merge with text
    w = len(text_)*size*0.62
    parts.insert(-1, f'<rect x="{x-w/2-3}" y="{y-size}" width="{w+6}" height="{size+6}" fill="#ffffff"/>')

parts.append('<defs><marker id="ah" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#000000"/></marker></defs>')
parts.append(f'<rect x="0" y="0" width="{W}" height="{H}" fill="#ffffff"/>')

CX = 220   # main spine center x
BW = 260   # box width
DW, DH = 240, 90  # diamond size

nodes = {}
def box(name,y,h,text_,shape="rect",cx=CX,w=BW):
    nodes[name] = dict(cx=cx,y=y,h=h,w=w,shape=shape)
    if shape=="rect":
        rect(cx-w/2,y,w,h)
        label(cx,y+h/2,text_)
    elif shape=="pill":
        pill(cx-w/2,y,w,h)
        label(cx,y+h/2,text_)
    elif shape=="diamond":
        diamond(cx,y+h/2,w,h)
        label(cx,y+h/2,text_)

box("A",30,44,"Power on","pill",w=180)
box("B",115,54,"Initialize LCD, HX711, RFID")
box("C",210,54,["Connect WiFi","(non-blocking)"])
box("D",305,54,"Show home screen")
box("E",400,DH,"RFID tag\nscanned?".split("\n"),"diamond")
box("F",535,DH,"Tag\nrecognized?".split("\n"),"diamond")
box("I",670,54,["Identify item,","check restock"])
box("K",760,54,["Update weight,","compute stock %"])
box("L1",850,54,"Update LCD")
box("M",940,54,["Upload to ThingSpeak","(scan-triggered)"])

# Diamond N after M
box("N",1015,DH,"Stock %\nbelow 20?".split("\n"),"diamond")

RX = 620   # column for F-no branch (G,H) and N-yes (O)
RX2 = 960  # separate column for E-no branch (J,P) - kept apart from RX to avoid collision
box("J",400,DH,"5-min upload\ntimer elapsed?".split("\n"),"diamond",cx=RX2,w=220)
box("P",540,54,["Upload all 4 items","(staggered 16s)"],cx=RX2,w=220)

box("G",535,54,"Show 'unknown item'",cx=RX,w=BW)
box("H",625,54,"Auto-return after 5s",cx=RX,w=BW)

box("O",1032,54,"Trigger low-stock alert",cx=RX,w=BW)

def edge(n): return nodes[n]

def bottom(n):
    d=nodes[n]; return (d['cx'], d['y']+d['h'])
def top(n):
    d=nodes[n]; return (d['cx'], d['y'])
def left(n):
    d=nodes[n]; return (d['cx']-d['w']/2, d['y']+d['h']/2)
def right(n):
    d=nodes[n]; return (d['cx']+d['w']/2, d['y']+d['h']/2)

# main spine
path([bottom('A'), top('B')])
path([bottom('B'), top('C')])
path([bottom('C'), top('D')])
path([bottom('D'), top('E')])
path([bottom('F'), top('I')])
path([bottom('I'), top('K')])
path([bottom('K'), top('L1')])
path([bottom('L1'), top('M')])
path([bottom('M'), top('N')])

# E -> F (yes, down)
path([bottom('E'), top('F')])
edge_label(nodes['E']['cx']+34, (bottom('E')[1]+top('F')[1])/2, "Yes")

# E -> J (no, right)
ex,ey = right('E')
jx,jy = left('J')
path([(ex,ey),(jx,jy)])
edge_label((ex+jx)/2, ey-10, "No")

# F -> I (yes) label near vertical segment already drawn
edge_label(nodes['F']['cx']+34, (bottom('F')[1]+top('I')[1])/2, "Yes")

# F -> G (no, right)
fx,fy = right('F')
gx,gy = left('G')
path([(fx,fy),(gx,gy)])
edge_label((fx+gx)/2, fy-10, "No")

# G -> H
path([bottom('G'), top('H')])

# --- shared return corridor: 5 lanes between RX column (ends 750) and RX2 column (starts 850) ---
lane_x  = [765,780,795,810,825]
enter_y = [313,321,329,337,345]
d_right = nodes['D']['cx']+BW/2

# H -> D
hx,hy = right('H')
path([(hx,hy),(lane_x[0],hy),(lane_x[0],enter_y[0]),(d_right,enter_y[0])])

# N -> O (yes, right)
nx,ny = right('N')
ox,oy = left('O')
path([(nx,ny),(ox,oy)])
edge_label((nx+ox)/2, ny-10, "Yes")

# N -> D (no, return via corridor)
nx2,ny2 = left('N')
path([(nx2,ny2),(lane_x[1],ny2),(lane_x[1],enter_y[1]),(d_right,enter_y[1])])
edge_label((nx2+lane_x[1])/2, ny2-10, "No")

# O -> D (return via corridor)
ox2,oy2 = right('O')
path([(ox2,oy2),(lane_x[2],oy2),(lane_x[2],enter_y[2]),(d_right,enter_y[2])])

# J -> P (yes, down)
path([bottom('J'), top('P')])
edge_label(nodes['J']['cx']+40, (bottom('J')[1]+top('P')[1])/2, "Yes")

# J -> D (no, return via corridor)
jlx,jly = left('J')
path([(jlx,jly),(lane_x[3],jly),(lane_x[3],enter_y[3]),(d_right,enter_y[3])])
edge_label((jlx+right('E')[0])/2, jly-24, "No")

# P -> D (return via corridor)
px,py = right('P')
path([(px,py),(lane_x[4],py),(lane_x[4],enter_y[4]),(d_right,enter_y[4])])

svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">\n' + "\n".join(parts) + '\n</svg>'
with open(r"C:\Users\Reedemer's\Desktop\flowchart.svg", "w", encoding="utf-8") as f:
    f.write(svg)
print("done")
