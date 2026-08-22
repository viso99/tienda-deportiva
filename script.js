// ---------- Datos de productos ----------
const PRODUCTS = {
  camisetas: [
    { id: 'cam-1', name: 'Camiseta Running Dry-Fit', price: 24.99, img: 'https://images.unsplash.com/photo-1516177609387-9bad55a45194?q=80&w=800&auto=format&fit=crop' },
    { id: 'cam-2', name: 'Camiseta Training Pro', price: 21.5, oldPrice: 29.99, img: 'https://images.unsplash.com/photo-1516442443906-71605254b628?q=80&w=800&auto=format&fit=crop' },
    { id: 'cam-3', name: 'Camiseta Manga Larga Térmica', price: 27.0, img: 'https://images.unsplash.com/photo-1573798484153-da43eda898f6?q=80&w=800&auto=format&fit=crop' },
    { id: 'cam-4', name: 'Camiseta Compresión Gym', price: 19.99, badge: 'Nuevo', img: 'https://images.unsplash.com/photo-1516082669438-2d2bb5082626?q=80&w=800&auto=format&fit=crop' },
    { id: 'cam-5', name: 'Camiseta Tirantes Atletismo', price: 17.5, img: 'https://images.unsplash.com/photo-1516178151140-1a27a08c417a?q=80&w=800&auto=format&fit=crop' },
    { id: 'cam-6', name: 'Camiseta Oversize Street', price: 22.0, img: 'https://images.unsplash.com/photo-1516258454449-64fed1caa732?q=80&w=800&auto=format&fit=crop' },
  ],
  pantalones: [
    { id: 'pan-1', name: 'Mallas Largas Running', price: 29.99, img: 'https://images.unsplash.com/photo-1594633312681-425c7b97ccd1?q=80&w=800&auto=format&fit=crop' },
    { id: 'pan-2', name: 'Jogger Deportivo Slim', price: 34.5, img: 'https://images.unsplash.com/photo-1473966968600-fa801b869a1a?q=80&w=800&auto=format&fit=crop' },
    { id: 'pan-3', name: 'Short Training Ligero', price: 18.99, badge: 'Nuevo', img: 'https://images.unsplash.com/photo-1706177208693-2e3c68e5f0f2?q=80&w=800&auto=format&fit=crop' },
    { id: 'pan-4', name: 'Pantalón Chándal Fleece', price: 39.0, oldPrice: 49.0, img: 'https://images.unsplash.com/photo-1715532098035-a343b26eaeaa?q=80&w=800&auto=format&fit=crop' },
    { id: 'pan-5', name: 'Mallas 3/4 Ciclismo', price: 26.5, img: 'https://images.unsplash.com/photo-1552904219-f4b87efe8792?q=80&w=800&auto=format&fit=crop' },
    { id: 'pan-6', name: 'Short Baloncesto Ancho', price: 22.99, img: 'https://images.unsplash.com/photo-1601561446301-fecc99036f4b?q=80&w=800&auto=format&fit=crop' },
  ],
  zapatillas: [
    { id: 'zap-1', name: 'Zapatillas Running Air Flow', price: 69.99, img: 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?q=80&w=800&auto=format&fit=crop' },
    { id: 'zap-2', name: 'Zapatillas Training Cross', price: 64.5, img: 'https://images.unsplash.com/photo-1606107557195-0e29a4b5b4aa?q=80&w=800&auto=format&fit=crop' },
    { id: 'zap-3', name: 'Zapatillas Baloncesto High-Top', price: 89.99, badge: 'Nuevo', img: 'https://images.unsplash.com/photo-1571008887538-b36bb32f4571?q=80&w=800&auto=format&fit=crop' },
    { id: 'zap-4', name: 'Zapatillas Fútbol Sala', price: 54.0, img: 'https://images.unsplash.com/photo-1560769629-975ec94e6a86?q=80&w=800&auto=format&fit=crop' },
    { id: 'zap-5', name: 'Zapatillas Casual Street', price: 47.99, oldPrice: 59.99, img: 'https://images.unsplash.com/photo-1590333748338-d629e4564ad9?q=80&w=800&auto=format&fit=crop' },
    { id: 'zap-6', name: 'Zapatillas Trail Running', price: 74.5, img: 'https://images.unsplash.com/photo-1491553895911-0055eca6402d?q=80&w=800&auto=format&fit=crop' },
  ],
  // Ligas y equipos ficticios (inspirados en las grandes ligas europeas) — sin marcas ni escudos reales.
  futbol: [
    { id: 'fut-1', league: 'Liga Ibérica', team: 'CF Toledo', price: 79.99, colors: ['#c0392b', '#f1c40f'], initials: 'CFT' },
    { id: 'fut-2', league: 'Liga Insular', team: 'Albion United', price: 79.99, colors: ['#1e3a8a', '#ffffff'], initials: 'AU' },
    { id: 'fut-3', league: 'Liga Adriática', team: 'AC Vesuvio', price: 89.99, badge: 'Nuevo', colors: ['#2c3e50', '#3498db'], initials: 'ACV' },
    { id: 'fut-4', league: 'Liga Bávara', team: 'FC Rheinstadt', price: 74.5, colors: ['#c0392b', '#1a1a1a'], initials: 'FCR' },
    { id: 'fut-5', league: 'Liga Galia', team: 'AS Provence', price: 69.99, oldPrice: 84.99, colors: ['#16a085', '#ffffff'], initials: 'ASP' },
    { id: 'fut-6', league: 'Liga Bataava', team: 'Oranje FC', price: 59.99, colors: ['#e67e22', '#1a1a1a'], initials: 'OFC' },
  ],
  // Ligas y equipos ficticios (inspirados en el baloncesto estilo NBA) — sin marcas ni escudos reales.
  baloncesto: [
    { id: 'bal-1', league: 'Conferencia Costa', team: 'Bay City Kraken', price: 84.99, colors: ['#0d47a1', '#00bcd4'], initials: 'BCK' },
    { id: 'bal-2', league: 'Conferencia Sur', team: 'Lonestar Mustangs', price: 84.99, colors: ['#6a1b9a', '#ffd54f'], initials: 'LM' },
    { id: 'bal-3', league: 'Conferencia Este', team: 'Capital Sentinels', price: 94.99, badge: 'Nuevo', colors: ['#1a1a2e', '#c0392b'], initials: 'CS' },
    { id: 'bal-4', league: 'Conferencia Central', team: 'Lakeside Comets', price: 54.99, colors: ['#004d40', '#ffffff'], initials: 'LC' },
    { id: 'bal-5', league: 'Conferencia Desierto', team: 'Desert Scorpions', price: 79.99, oldPrice: 94.99, colors: ['#bf360c', '#212121'], initials: 'DS' },
    { id: 'bal-6', league: 'Conferencia Metro', team: 'Metro Titans', price: 49.99, colors: ['#37474f', '#f9a825'], initials: 'MT' },
  ],
};

const CATEGORY_LABEL = {
  camisetas: 'Camisetas',
  pantalones: 'Pantalones',
  zapatillas: 'Zapatillas',
};

const fmt = (n) => n.toLocaleString('es-ES', { minimumFractionDigits: 2, maximumFractionDigits: 2 }) + ' €';

// ---------- Escudo/camiseta ilustrados por equipo (fútbol/baloncesto) ----------
function teamCrestSVG(p, cat) {
  const [colorA, colorB] = p.colors;
  const jerseyPoints =
    cat === 'baloncesto'
      ? '42,8 58,8 58,16 72,20 68,34 62,28 62,92 38,92 38,28 32,34 28,20 42,16'
      : '38,8 62,8 62,14 70,10 88,26 76,40 68,32 68,92 32,92 32,32 24,40 12,26 30,10 38,14';

  const ballBadge =
    cat === 'baloncesto'
      ? `<circle cx="80" cy="80" r="13" fill="#e8590c"/>
         <line x1="67" y1="80" x2="93" y2="80" stroke="#1a1a1a" stroke-width="1.4"/>
         <line x1="80" y1="67" x2="80" y2="93" stroke="#1a1a1a" stroke-width="1.4"/>
         <path d="M70,71 Q80,80 70,89" fill="none" stroke="#1a1a1a" stroke-width="1.4"/>
         <path d="M90,71 Q80,80 90,89" fill="none" stroke="#1a1a1a" stroke-width="1.4"/>`
      : `<circle cx="82" cy="82" r="13" fill="#fff" stroke="rgba(0,0,0,.25)" stroke-width="1.4"/>
         <path d="M82,71 L87,76 L85,81 L79,81 L77,76 Z" fill="rgba(0,0,0,.35)"/>`;

  return `
    <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
      <polygon points="${jerseyPoints}" fill="${colorA}"/>
      <rect x="32" y="18" width="36" height="6" fill="${colorB}" opacity=".85"/>
      <circle cx="50" cy="46" r="15" fill="${colorB}" stroke="#fff" stroke-width="2"/>
      <text x="50" y="51" font-size="13" font-weight="800" text-anchor="middle" fill="${colorA}" font-family="Arial, sans-serif">${p.initials}</text>
      ${ballBadge}
    </svg>`;
}

// ---------- Render de productos ----------
function renderProducts() {
  document.querySelectorAll('.product-grid').forEach((grid) => {
    const cat = grid.dataset.category;
    const items = PRODUCTS[cat] || [];
    grid.innerHTML = items.map((p) => productCardHTML(p, cat)).join('');
  });
}

function isKit(cat) {
  return cat === 'futbol' || cat === 'baloncesto';
}

function productCardHTML(p, cat) {
  const badge = p.badge
    ? `<span class="card-badge">${p.badge}</span>`
    : p.oldPrice
    ? `<span class="card-badge sale">Oferta</span>`
    : '';
  const priceHTML = p.oldPrice
    ? `<span class="old">${fmt(p.oldPrice)}</span>${fmt(p.price)}`
    : fmt(p.price);

  const media = isKit(cat)
    ? `<div class="card-media kit-media" style="background:linear-gradient(135deg, ${p.colors[0]} 0%, #14161c 85%)">${badge}${teamCrestSVG(p, cat)}</div>`
    : `<div class="card-media"><img src="${p.img}" alt="${p.name}" loading="lazy">${badge}</div>`;

  const catLabel = isKit(cat) ? p.league : CATEGORY_LABEL[cat];
  const name = isKit(cat) ? `Equipación ${p.team}` : p.name;

  return `
    <article class="card">
      ${media}
      <div class="card-body">
        <span class="card-cat">${catLabel}</span>
        <h3 class="card-name">${name}</h3>
        <div class="card-price-row">
          <span class="card-price">${priceHTML}</span>
          <button class="add-btn" data-id="${p.id}" data-cat="${cat}">Añadir</button>
        </div>
      </div>
    </article>
  `;
}

// ---------- Carrito ----------
let cart = [];

function findProduct(id) {
  for (const cat in PRODUCTS) {
    const found = PRODUCTS[cat].find((p) => p.id === id);
    if (found) return { ...found, cat };
  }
  return null;
}

function addToCart(id) {
  const existing = cart.find((item) => item.id === id);
  if (existing) {
    existing.qty += 1;
  } else {
    const product = findProduct(id);
    if (!product) return;
    cart.push({ ...product, qty: 1 });
  }
  renderCart();
}

function changeQty(id, delta) {
  const item = cart.find((i) => i.id === id);
  if (!item) return;
  item.qty += delta;
  if (item.qty <= 0) {
    cart = cart.filter((i) => i.id !== id);
  }
  renderCart();
}

function removeFromCart(id) {
  cart = cart.filter((i) => i.id !== id);
  renderCart();
}

function cartItemName(item) {
  return isKit(item.cat) ? `Equipación ${item.team}` : item.name;
}

function cartItemMedia(item) {
  return isKit(item.cat)
    ? `<div class="cart-item-media" style="background:linear-gradient(135deg, ${item.colors[0]} 0%, #14161c 85%)">${teamCrestSVG(item, item.cat)}</div>`
    : `<div class="cart-item-media"><img src="${item.img}" alt="${item.name}"></div>`;
}

function renderCart() {
  const container = document.getElementById('cartItems');
  const totalEl = document.getElementById('cartTotal');
  const countEl = document.getElementById('cartCount');

  const totalItems = cart.reduce((sum, i) => sum + i.qty, 0);
  const totalPrice = cart.reduce((sum, i) => sum + i.qty * i.price, 0);

  countEl.textContent = totalItems;
  totalEl.textContent = fmt(totalPrice);

  if (cart.length === 0) {
    container.innerHTML = '<p class="cart-empty">Tu carrito está vacío. ¡Añade algún producto!</p>';
    return;
  }

  container.innerHTML = cart
    .map(
      (item) => `
    <div class="cart-item">
      ${cartItemMedia(item)}
      <div class="cart-item-info">
        <p>${cartItemName(item)}</p>
        <span>${fmt(item.price)}</span>
        <div class="cart-item-qty">
          <button data-action="dec" data-id="${item.id}">−</button>
          <span>${item.qty}</span>
          <button data-action="inc" data-id="${item.id}">+</button>
        </div>
      </div>
      <button class="remove-item" data-action="remove" data-id="${item.id}" aria-label="Eliminar">✕</button>
    </div>
  `
    )
    .join('');
}

// ---------- Eventos ----------
function setupEvents() {
  // Añadir al carrito (delegación de eventos)
  document.querySelectorAll('.product-grid').forEach((grid) => {
    grid.addEventListener('click', (e) => {
      const btn = e.target.closest('.add-btn');
      if (!btn) return;
      addToCart(btn.dataset.id);
      btn.textContent = 'Añadido ✓';
      btn.classList.add('added');
      setTimeout(() => {
        btn.textContent = 'Añadir';
        btn.classList.remove('added');
      }, 900);
    });
  });

  // Acciones dentro del carrito
  document.getElementById('cartItems').addEventListener('click', (e) => {
    const btn = e.target.closest('button[data-action]');
    if (!btn) return;
    const { action, id } = btn.dataset;
    if (action === 'inc') changeQty(id, 1);
    if (action === 'dec') changeQty(id, -1);
    if (action === 'remove') removeFromCart(id);
  });

  // Abrir / cerrar carrito
  const drawer = document.getElementById('cartDrawer');
  const overlay = document.getElementById('cartOverlay');
  const openCart = () => {
    drawer.classList.add('open');
    overlay.classList.add('open');
  };
  const closeCart = () => {
    drawer.classList.remove('open');
    overlay.classList.remove('open');
  };
  document.getElementById('cartBtn').addEventListener('click', openCart);
  document.getElementById('closeCart').addEventListener('click', closeCart);
  overlay.addEventListener('click', closeCart);

  // Checkout (demo)
  document.getElementById('checkoutBtn').addEventListener('click', () => {
    if (cart.length === 0) {
      alert('Tu carrito está vacío.');
      return;
    }
    alert('¡Gracias por tu compra! (demo — no se ha procesado ningún pago real)');
    cart = [];
    renderCart();
    closeCart();
  });

  // Menú móvil
  const burger = document.getElementById('burger');
  const nav = document.getElementById('mainNav');
  burger.addEventListener('click', () => nav.classList.toggle('open'));
  nav.querySelectorAll('a').forEach((a) =>
    a.addEventListener('click', () => nav.classList.remove('open'))
  );
}

// ---------- Init ----------
renderProducts();
renderCart();
setupEvents();
