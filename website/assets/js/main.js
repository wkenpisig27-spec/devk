/**
 * PKO Website - Main JavaScript
 * Handles UI interactions, animations, and API calls
 */

'use strict';

// ============================================================================
// CONFIGURATION
// ============================================================================

const PKO = {
    apiBase: '/api',
    toastDuration: 5000,
    animationDuration: 300
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * DOM Ready helper
 */
function ready(fn) {
    if (document.readyState !== 'loading') {
        fn();
    } else {
        document.addEventListener('DOMContentLoaded', fn);
    }
}

/**
 * Query selector helper
 */
function $(selector, parent = document) {
    return parent.querySelector(selector);
}

function $$(selector, parent = document) {
    return Array.from(parent.querySelectorAll(selector));
}

/**
 * Create element helper
 */
function createElement(tag, attrs = {}, children = []) {
    const el = document.createElement(tag);
    Object.entries(attrs).forEach(([key, value]) => {
        if (key === 'className') {
            el.className = value;
        } else if (key === 'dataset') {
            Object.entries(value).forEach(([k, v]) => el.dataset[k] = v);
        } else if (key.startsWith('on')) {
            el.addEventListener(key.slice(2).toLowerCase(), value);
        } else {
            el.setAttribute(key, value);
        }
    });
    children.forEach(child => {
        if (typeof child === 'string') {
            el.appendChild(document.createTextNode(child));
        } else if (child) {
            el.appendChild(child);
        }
    });
    return el;
}

/**
 * Debounce function
 */
function debounce(fn, delay) {
    let timeout;
    return function (...args) {
        clearTimeout(timeout);
        timeout = setTimeout(() => fn.apply(this, args), delay);
    };
}

/**
 * Throttle function
 */
function throttle(fn, limit) {
    let inThrottle;
    return function (...args) {
        if (!inThrottle) {
            fn.apply(this, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}

/**
 * Format number with commas
 */
function formatNumber(num) {
    return num.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

/**
 * Escape HTML special characters
 */
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// ============================================================================
// API CLIENT
// ============================================================================

const API = {
    /**
     * Make API request
     */
    async request(endpoint, options = {}) {
        const url = `${PKO.apiBase}${endpoint}`;

        const config = {
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json',
                ...options.headers
            },
            ...options
        };

        // Add auth token if available
        const token = localStorage.getItem('pko_token');
        if (token) {
            config.headers['Authorization'] = `Bearer ${token}`;
        }

        // Add CSRF token if available
        const csrfToken = document.querySelector('meta[name="csrf-token"]')?.content;
        if (csrfToken) {
            config.headers['X-CSRF-Token'] = csrfToken;
        }

        try {
            const response = await fetch(url, config);
            const data = await response.json();

            if (!response.ok) {
                throw new Error(data.error || 'An error occurred');
            }

            return data;
        } catch (error) {
            console.error('API Error:', error);
            throw error;
        }
    },

    get(endpoint) {
        return this.request(endpoint, { method: 'GET' });
    },

    post(endpoint, body) {
        return this.request(endpoint, {
            method: 'POST',
            body: JSON.stringify(body)
        });
    },

    put(endpoint, body) {
        return this.request(endpoint, {
            method: 'PUT',
            body: JSON.stringify(body)
        });
    },

    delete(endpoint) {
        return this.request(endpoint, { method: 'DELETE' });
    }
};

// ============================================================================
// TOAST NOTIFICATIONS
// ============================================================================

const Toast = {
    container: null,

    init() {
        this.container = document.createElement('div');
        this.container.className = 'toast-container';
        document.body.appendChild(this.container);
    },

    show(message, type = 'info', duration = PKO.toastDuration) {
        if (!this.container) this.init();

        const icons = {
            success: '<svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>',
            error: '<svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clip-rule="evenodd"/></svg>',
            warning: '<svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd"/></svg>',
            info: '<svg width="20" height="20" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/></svg>'
        };

        const colors = {
            success: 'color: #10b981',
            error: 'color: #ef4444',
            warning: 'color: #f59e0b',
            info: 'color: #3b82f6'
        };

        const toast = createElement('div', { className: 'toast' }, [
            createElement('span', { style: colors[type] }),
            createElement('span', { className: 'toast-message' }, [message])
        ]);

        toast.firstChild.innerHTML = icons[type] || icons.info;

        // Add close button
        const closeBtn = createElement('button', {
            className: 'toast-close',
            onClick: () => this.remove(toast)
        });
        closeBtn.innerHTML = '&times;';
        toast.appendChild(closeBtn);

        this.container.appendChild(toast);

        // Auto remove
        if (duration > 0) {
            setTimeout(() => this.remove(toast), duration);
        }

        return toast;
    },

    remove(toast) {
        toast.style.animation = 'slideOut 0.3s ease-out forwards';
        setTimeout(() => toast.remove(), 300);
    },

    success(message) { return this.show(message, 'success'); },
    error(message) { return this.show(message, 'error'); },
    warning(message) { return this.show(message, 'warning'); },
    info(message) { return this.show(message, 'info'); }
};

// Add slide out animation
const toastStyle = document.createElement('style');
toastStyle.textContent = `
    @keyframes slideOut {
        to {
            opacity: 0;
            transform: translateX(100%);
        }
    }
    .toast-close {
        margin-left: auto;
        background: none;
        border: none;
        color: inherit;
        opacity: 0.5;
        cursor: pointer;
        font-size: 1.25rem;
        line-height: 1;
    }
    .toast-close:hover { opacity: 1; }
`;
document.head.appendChild(toastStyle);

// ============================================================================
// MODAL SYSTEM
// ============================================================================

const Modal = {
    current: null,

    open(id) {
        const modal = document.getElementById(id);
        const backdrop = document.getElementById(id + '-backdrop') || this.createBackdrop(id);

        if (modal) {
            modal.classList.add('open');
            backdrop.classList.add('open');
            document.body.style.overflow = 'hidden';
            this.current = modal;

            // Focus trap
            modal.setAttribute('tabindex', '-1');
            modal.focus();
        }
    },

    close(id = null) {
        const modal = id ? document.getElementById(id) : this.current;
        if (modal) {
            modal.classList.remove('open');
            const backdrop = document.getElementById(modal.id + '-backdrop');
            if (backdrop) backdrop.classList.remove('open');
            document.body.style.overflow = '';
            this.current = null;
        }
    },

    createBackdrop(id) {
        const backdrop = createElement('div', {
            id: id + '-backdrop',
            className: 'modal-backdrop',
            onClick: () => this.close(id)
        });
        document.body.appendChild(backdrop);
        return backdrop;
    }
};

// ============================================================================
// NAVBAR
// ============================================================================

function initNavbar() {
    const navbar = $('.navbar');
    if (!navbar) return;

    // Scroll effect
    const onScroll = throttle(() => {
        if (window.scrollY > 50) {
            navbar.classList.add('scrolled');
        } else {
            navbar.classList.remove('scrolled');
        }
    }, 100);

    window.addEventListener('scroll', onScroll);
    onScroll(); // Initial check

    // Mobile menu toggle
    const toggle = $('.navbar-toggle');
    const menu = $('.navbar-menu');

    if (toggle && menu) {
        toggle.addEventListener('click', () => {
            menu.classList.toggle('open');
            toggle.setAttribute('aria-expanded', menu.classList.contains('open'));
        });

        // Close on click outside
        document.addEventListener('click', (e) => {
            if (!menu.contains(e.target) && !toggle.contains(e.target)) {
                menu.classList.remove('open');
                toggle.setAttribute('aria-expanded', 'false');
            }
        });

        // Close on menu link click
        $$('.navbar-link', menu).forEach(link => {
            link.addEventListener('click', () => {
                menu.classList.remove('open');
                toggle.setAttribute('aria-expanded', 'false');
            });
        });
    }
}

// ============================================================================
// HERO PARTICLES
// ============================================================================

function initParticles() {
    const container = $('.hero-particles');
    if (!container) return;

    const particleCount = 30;

    for (let i = 0; i < particleCount; i++) {
        const particle = createElement('div', { className: 'hero-particle' });
        particle.style.left = Math.random() * 100 + '%';
        particle.style.animationDelay = Math.random() * 8 + 's';
        particle.style.animationDuration = (6 + Math.random() * 4) + 's';
        container.appendChild(particle);
    }
}

// ============================================================================
// ANIMATED COUNTERS
// ============================================================================

function initCounters() {
    const counters = $$('[data-counter]');
    if (!counters.length) return;

    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                animateCounter(entry.target);
                observer.unobserve(entry.target);
            }
        });
    }, { threshold: 0.5 });

    counters.forEach(counter => observer.observe(counter));
}

function animateCounter(element) {
    const target = parseInt(element.dataset.counter, 10);
    const duration = parseInt(element.dataset.duration, 10) || 2000;
    const start = 0;
    const startTime = performance.now();

    function update(currentTime) {
        const elapsed = currentTime - startTime;
        const progress = Math.min(elapsed / duration, 1);

        // Easing function (ease-out cubic)
        const easeOut = 1 - Math.pow(1 - progress, 3);
        const current = Math.floor(start + (target - start) * easeOut);

        element.textContent = formatNumber(current);

        if (progress < 1) {
            requestAnimationFrame(update);
        }
    }

    requestAnimationFrame(update);
}

// ============================================================================
// FORM HANDLING
// ============================================================================

function initForms() {
    // Login form
    const loginForm = $('#login-form');
    if (loginForm) {
        loginForm.addEventListener('submit', handleLogin);
    }

    // Register form
    const registerForm = $('#register-form');
    if (registerForm) {
        registerForm.addEventListener('submit', handleRegister);
    }

    // Password strength indicator
    const passwordInputs = $$('input[type="password"][data-strength]');
    passwordInputs.forEach(input => {
        input.addEventListener('input', () => updatePasswordStrength(input));
    });

    // Form validation
    $$('form[data-validate]').forEach(form => {
        form.addEventListener('submit', (e) => {
            if (!validateForm(form)) {
                e.preventDefault();
            }
        });
    });
}

async function handleLogin(e) {
    e.preventDefault();

    const form = e.target;
    const submitBtn = form.querySelector('button[type="submit"]');
    const originalText = submitBtn.innerHTML;

    // Disable button and show loading
    submitBtn.disabled = true;
    submitBtn.innerHTML = '<span class="loading"></span>';

    try {
        const formData = new FormData(form);
        const data = {
            username: formData.get('username'),
            password: formData.get('password'),
            remember: formData.get('remember') === 'on'
        };

        const response = await API.post('/auth/login.php', data);

        if (response.success) {
            // Store token
            if (response.token) {
                localStorage.setItem('pko_token', response.token);
            }

            Toast.success('Login successful! Redirecting...');

            // Redirect
            let redirect = new URLSearchParams(window.location.search).get('redirect') || '/dashboard.php';

            // Auto redirect admins to admin panel
            if (response.user && response.user.gm_level >= 99) {
                redirect = '/admin/';
            }

            setTimeout(() => window.location.href = redirect, 1000);
        }
    } catch (error) {
        Toast.error(error.message || 'Login failed. Please try again.');
    } finally {
        submitBtn.disabled = false;
        submitBtn.innerHTML = originalText;
    }
}

async function handleRegister(e) {
    e.preventDefault();

    const form = e.target;
    const submitBtn = form.querySelector('button[type="submit"]');
    const originalText = submitBtn.innerHTML;

    // Validate form first
    if (!validateForm(form)) {
        return;
    }

    // Disable button and show loading
    submitBtn.disabled = true;
    submitBtn.innerHTML = '<span class="loading"></span>';

    try {
        const formData = new FormData(form);
        const data = {
            username: formData.get('username'),
            password: formData.get('password'),
            password_confirm: formData.get('password_confirm'),
            email: formData.get('email'),
            csrf_token: formData.get('csrf_token')
        };

        const response = await API.post('/auth/register.php', data);

        if (response.success) {
            Toast.success('Registration successful! Please log in.');
            setTimeout(() => window.location.href = '/login.php', 1500);
        }
    } catch (error) {
        Toast.error(error.message || 'Registration failed. Please try again.');
    } finally {
        submitBtn.disabled = false;
        submitBtn.innerHTML = originalText;
    }
}

function validateForm(form) {
    let isValid = true;

    // Clear previous errors
    $$('.form-error', form).forEach(el => el.remove());
    $$('.error', form).forEach(el => el.classList.remove('error'));

    // Required fields
    $$('[required]', form).forEach(input => {
        if (!input.value.trim()) {
            showFieldError(input, 'This field is required');
            isValid = false;
        }
    });

    // Email validation
    $$('input[type="email"]', form).forEach(input => {
        if (input.value && !isValidEmail(input.value)) {
            showFieldError(input, 'Please enter a valid email address');
            isValid = false;
        }
    });

    // Password confirmation
    const password = $('input[name="password"]', form);
    const confirm = $('input[name="password_confirm"]', form);
    if (password && confirm && password.value !== confirm.value) {
        showFieldError(confirm, 'Passwords do not match');
        isValid = false;
    }

    // Password strength
    if (password && password.value && password.value.length < 6) {
        showFieldError(password, 'Password must be at least 6 characters');
        isValid = false;
    }

    return isValid;
}

function showFieldError(input, message) {
    input.classList.add('error');
    const error = createElement('div', { className: 'form-error' }, [message]);
    input.parentNode.appendChild(error);
}

function isValidEmail(email) {
    return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
}

function updatePasswordStrength(input) {
    const strengthBar = $(`#${input.dataset.strength}`);
    if (!strengthBar) return;

    const password = input.value;
    let strength = 0;

    if (password.length >= 6) strength++;
    if (password.length >= 10) strength++;
    if (/[a-z]/.test(password) && /[A-Z]/.test(password)) strength++;
    if (/[0-9]/.test(password)) strength++;
    if (/[^a-zA-Z0-9]/.test(password)) strength++;

    const percentage = (strength / 5) * 100;
    strengthBar.style.width = percentage + '%';

    strengthBar.className = 'progress-bar';
    if (strength <= 2) strengthBar.classList.add('danger');
    else if (strength <= 3) strengthBar.classList.add('warning');
    else strengthBar.classList.add('success');
}

// ============================================================================
// TABS
// ============================================================================

function initTabs() {
    $$('[data-tabs]').forEach(container => {
        const tabs = $$('[data-tab]', container);
        const panels = $$('[data-tab-panel]', container);

        tabs.forEach(tab => {
            tab.addEventListener('click', () => {
                const target = tab.dataset.tab;

                // Update tabs
                tabs.forEach(t => t.classList.remove('active'));
                tab.classList.add('active');

                // Update panels
                panels.forEach(panel => {
                    if (panel.dataset.tabPanel === target) {
                        panel.classList.remove('hidden');
                    } else {
                        panel.classList.add('hidden');
                    }
                });
            });
        });
    });
}

// ============================================================================
// LEADERBOARD
// ============================================================================

async function initLeaderboard() {
    const container = $('.leaderboard');
    if (!container) return;

    const tabs = $$('.leaderboard-tab', container);
    const list = $('.leaderboard-list', container);

    tabs.forEach(tab => {
        tab.addEventListener('click', async () => {
            // Update active tab
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            // Load data
            const type = tab.dataset.leaderboard;
            await loadLeaderboard(list, type);
        });
    });

    // Load initial data
    const activeTab = $('.leaderboard-tab.active', container);
    if (activeTab) {
        await loadLeaderboard(list, activeTab.dataset.leaderboard);
    }
}

async function loadLeaderboard(container, type) {
    container.innerHTML = '<li class="leaderboard-item"><div class="skeleton" style="height: 40px; width: 100%;"></div></li>'.repeat(10);

    try {
        const response = await API.get(`/leaderboard.php?type=${type}&limit=10`);

        if (response.success && response.data) {
            container.innerHTML = response.data.map((player, index) => `
                <li class="leaderboard-item">
                    <span class="leaderboard-rank ${getRankClass(index)}">${index + 1}</span>
                    <img class="leaderboard-avatar" src="/assets/images/classes/${escapeHtml((player.job || 'newbie').toLowerCase().replace(/ /g, '_'))}.svg" alt="" onerror="this.src='/assets/images/classes/newbie.svg'">
                    <div class="leaderboard-info">
                        <div class="leaderboard-name">${escapeHtml(player.cha_name)}</div>
                        <div class="leaderboard-meta">${escapeHtml(player.guild_name || 'No Guild')} • Lv.${player.degree}</div>
                    </div>
                    <span class="leaderboard-value">${formatNumber(player[type === 'level' ? 'exp' : 'battle_power'])}</span>
                </li>
            `).join('');
        }
    } catch (error) {
        container.innerHTML = '<li class="leaderboard-item text-center text-muted">Failed to load leaderboard</li>';
    }
}

function getRankClass(index) {
    if (index === 0) return 'gold';
    if (index === 1) return 'silver';
    if (index === 2) return 'bronze';
    return '';
}

// ============================================================================
// SERVER STATUS
// ============================================================================

async function initServerStatus() {
    const statusElements = $$('[data-server-status]');
    if (!statusElements.length) return;

    try {
        const response = await API.get('/status.php');

        statusElements.forEach(el => {
            const dot = $('.server-status-dot', el);
            const text = el.querySelector('span:last-child');

            if (response.online) {
                dot?.classList.add('online');
                dot?.classList.remove('offline');
                if (text) text.textContent = `Online • ${formatNumber(response.players_online)} Players`;
            } else {
                dot?.classList.add('offline');
                dot?.classList.remove('online');
                if (text) text.textContent = 'Offline';
            }
        });

        // Update stats
        if (response.total_accounts) {
            const accountCounter = $('[data-counter="accounts"]');
            if (accountCounter) {
                accountCounter.dataset.counter = response.total_accounts;
            }
        }
    } catch (error) {
        console.error('Failed to fetch server status:', error);
    }
}

// ============================================================================
// SMOOTH SCROLL
// ============================================================================

function initSmoothScroll() {
    $$('a[href^="#"]').forEach(link => {
        link.addEventListener('click', (e) => {
            const target = document.querySelector(link.getAttribute('href'));
            if (target) {
                e.preventDefault();
                const offset = 80; // Navbar height
                const top = target.getBoundingClientRect().top + window.pageYOffset - offset;

                window.scrollTo({
                    top,
                    behavior: 'smooth'
                });
            }
        });
    });
}

// ============================================================================
// SCROLL ANIMATIONS
// ============================================================================

function initScrollAnimations() {
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('animate-in');
                observer.unobserve(entry.target);
            }
        });
    }, {
        threshold: 0.1,
        rootMargin: '0px 0px -50px 0px'
    });

    $$('[data-animate]').forEach(el => {
        el.classList.add('animate-prepare');
        observer.observe(el);
    });
}

// Add animation styles
const animationStyles = document.createElement('style');
animationStyles.textContent = `
    .animate-prepare {
        opacity: 0;
        transform: translateY(30px);
    }
    .animate-in {
        animation: animateIn 0.6s ease-out forwards;
    }
    @keyframes animateIn {
        to {
            opacity: 1;
            transform: translateY(0);
        }
    }
`;
document.head.appendChild(animationStyles);

// ============================================================================
// COPY TO CLIPBOARD
// ============================================================================

function initCopyButtons() {
    $$('[data-copy]').forEach(btn => {
        btn.addEventListener('click', async () => {
            const text = btn.dataset.copy || btn.textContent;

            try {
                await navigator.clipboard.writeText(text);
                Toast.success('Copied to clipboard!');
            } catch (error) {
                // Fallback
                const textarea = document.createElement('textarea');
                textarea.value = text;
                document.body.appendChild(textarea);
                textarea.select();
                document.execCommand('copy');
                textarea.remove();
                Toast.success('Copied to clipboard!');
            }
        });
    });
}

// ============================================================================
// LOGOUT
// ============================================================================

function initLogout() {
    $$('[data-logout]').forEach(btn => {
        btn.addEventListener('click', async (e) => {
            e.preventDefault();

            try {
                await API.post('/auth/logout.php');
            } catch (error) {
                // Ignore errors
            }

            localStorage.removeItem('pko_token');
            window.location.href = '/';
        });
    });
}

// ============================================================================
// CHARACTER SELECTOR
// ============================================================================

function initCharacterSelector() {
    const selector = $('[data-character-selector]');
    if (!selector) return;

    const cards = $$('.character-card', selector);
    const hiddenInput = $('input[name="character_id"]');

    cards.forEach(card => {
        card.addEventListener('click', () => {
            cards.forEach(c => c.classList.remove('selected'));
            card.classList.add('selected');
            if (hiddenInput) {
                hiddenInput.value = card.dataset.characterId;
            }
        });
    });
}

// ============================================================================
// VOTE SYSTEM
// ============================================================================

async function handleVote(siteId) {
    const btn = document.querySelector(`[data-vote-site="${siteId}"]`);
    if (!btn || btn.disabled) return;

    try {
        const response = await API.post('/vote.php', { site_id: siteId });

        if (response.success) {
            Toast.success('Vote recorded! Redirecting to vote site...');

            // Disable button temporarily
            btn.disabled = true;
            btn.textContent = 'Voted!';

            // Open vote link
            if (response.vote_link) {
                window.open(response.vote_link, '_blank');
            }

            // Refresh cooldown display
            if (response.next_vote) {
                updateVoteCooldown(siteId, response.next_vote);
            }
        }
    } catch (error) {
        Toast.error(error.message || 'Failed to record vote. Please try again.');
    }
}

function updateVoteCooldown(siteId, nextVoteTime) {
    const btn = document.querySelector(`[data-vote-site="${siteId}"]`);
    if (!btn) return;

    const updateTimer = () => {
        const now = Date.now();
        const remaining = nextVoteTime - now;

        if (remaining <= 0) {
            btn.disabled = false;
            btn.textContent = 'Vote';
            return;
        }

        const hours = Math.floor(remaining / 3600000);
        const minutes = Math.floor((remaining % 3600000) / 60000);

        btn.textContent = `${hours}h ${minutes}m`;
        btn.disabled = true;

        setTimeout(updateTimer, 60000);
    };

    updateTimer();
}

// ============================================================================
// SHOP SYSTEM
// ============================================================================

async function handlePurchase(itemId) {
    const characterId = $('input[name="character_id"]')?.value;

    if (!characterId) {
        Toast.warning('Please select a character first.');
        Modal.open('character-modal');
        return;
    }

    // Confirm purchase
    if (!confirm('Are you sure you want to purchase this item?')) {
        return;
    }

    try {
        const response = await API.post('/shop/purchase.php', {
            item_id: itemId,
            character_id: characterId
        });

        if (response.success) {
            Toast.success(response.message || 'Purchase successful!');

            // Update credit display
            if (response.new_balance !== undefined) {
                const creditDisplay = $('[data-credits]');
                if (creditDisplay) {
                    creditDisplay.textContent = formatNumber(response.new_balance);
                }
            }

            // Refresh page to update quantities
            setTimeout(() => window.location.reload(), 1500);
        }
    } catch (error) {
        Toast.error(error.message || 'Purchase failed. Please try again.');
    }
}

// Expose to global scope for inline handlers
window.handlePurchase = handlePurchase;
window.handleVote = handleVote;

// ============================================================================
// INITIALIZATION
// ============================================================================

ready(() => {
    // Core UI
    initNavbar();
    initParticles();
    initCounters();
    initTabs();
    initSmoothScroll();
    initScrollAnimations();

    // Forms & Auth
    initForms();
    initLogout();

    // Features
    initLeaderboard();
    initServerStatus();
    initCopyButtons();
    initCharacterSelector();

    // Modal close on Escape
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            Modal.close();
        }
    });

    // Initialize toast system
    Toast.init();

    console.log('🏴‍☠️ PKO Website initialized!');
});

// Export for use in other scripts
window.PKO = {
    API,
    Toast,
    Modal,
    $,
    $$,
    formatNumber,
    escapeHtml
};
