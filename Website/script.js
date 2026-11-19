
var angle = 180
var direction = 1

var UltrasonicReadings = []

for (let i = 0; i < 90; i++) {
  UltrasonicReadings += Math.floor(Math.random() * 100);
}

var canvas = document.getElementById("UltrasonicCanvas")
canvas.width = 500
canvas.height = 300

var ctx = canvas.getContext("2d")

function drawPoint(i) {
  const e = UltrasonicReadings[i];
  ctx.beginPath()
  ctx.moveTo(250, 280)
  ctx.lineTo(
    250 + Math.sin((i + 90) * Math.PI / 180) * e * 20,
    280 + Math.cos((i + 90) * Math.PI / 180) * e * 20
  )
  ctx.stroke()
}

function draw() {
  ctx.fillStyle = "black"
  ctx.fillRect(0, 0, 500, 300)

  // Radar arcs
  ctx.strokeStyle = "darkgreen"
  ctx.lineWidth = 1

  var circles = [50, 135, 225]

  circles.forEach(e => {
    ctx.beginPath()
    ctx.arc(250, 280, e, Math.PI, 0)
    ctx.stroke()
  })

  // Base line
  ctx.beginPath()
  ctx.moveTo(25, 280)
  ctx.lineTo(475, 280)
  ctx.stroke()

  // Scanning line
  var angleRad = angle * Math.PI / 180

  ctx.strokeStyle = "limegreen"
  ctx.lineWidth = 3

  ctx.beginPath()
  ctx.moveTo(250, 280)
  ctx.lineTo(
    250 + Math.sin(angleRad) * 225,
    280 + Math.cos(angleRad) * 225
  )
  ctx.stroke()

  if (direction == 1) {
    for (let i = 0; i < angle - 90; i++) {
      drawPoint(i);
    }
  }
  else {
    for (let i = 270; i >= angle - 90; i--) {
      drawPoint(i)
    }
  }

  // Update angle
  angle += direction
  console.log(angle)

  if (angle >= 270 || angle <= 90) {
    direction *= -1
  }

  requestAnimationFrame(draw)
}

draw()
