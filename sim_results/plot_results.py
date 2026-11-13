import csv
import os
import matplotlib.pyplot as plt

base_dir = os.path.expanduser("~/ros2_ws/sim_results")

def load_csv(name):
    t, x, v = [], [], []
    with open(os.path.join(base_dir, name), "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            t.append(float(row["t"]))
            x.append(float(row["x"]))
            v.append(float(row["v"]))
    return t, x, v

# Графік 1: Порівняння 3 основних режимів (без затримок, із затримкою, з предиктором)
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)

cases = [
    ("case1_no_delay.csv", "1. Без затримок (Δt_loop = 0 мс)", "tab:blue", "-"),
    ("case2_with_delay.csv", "2. Із затримкою (Δt_loop = 150 мс, без компенсації)", "tab:red", "--"),
    ("case3_compensated.csv", "3. Лінійний предиктор (Δt_loop = 150 мс, з компенсацією)", "tab:green", "-"),
]

for fname, label, color, style in cases:
    t, x, v = load_csv(fname)
    ax1.plot(t, x, label=label, color=color, linestyle=style, linewidth=2)
    ax2.plot(t, v, label=label, color=color, linestyle=style, linewidth=2)

ax1.axhline(0.05, color="black", linestyle=":", linewidth=1.2, label="Цільове положення x_target = 0.05 рад")
ax1.set_ylabel("Положення x, рад")
ax1.set_title("Перехідний процес П-регулятора суглоба (Kp=5.0, T=50 мс, a_max=0.5 рад/с²)")
ax1.grid(True, alpha=0.4)
ax1.legend(loc="lower right")

ax2.axhline(0.0, color="black", linestyle=":", linewidth=1.0)
ax2.set_xlabel("Час t, с")
ax2.set_ylabel("Швидкість v, рад/с")
ax2.grid(True, alpha=0.4)
ax2.legend(loc="upper right")

plt.tight_layout()
plt.savefig(os.path.join(base_dir, "fig1_main_comparison.png"), dpi=200)
plt.close()

# Графік 2: Вплив варіації Kp, T та Δt_loop (без предиктора)
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)

vars_list = [
    ("case2_with_delay.csv", "Базовий (Kp=5.0, T=50 мс, Δt_loop=150 мс)", "tab:red", "-"),
    ("var_kp_2.csv", "Зменшений Kp=2.0 (T=50 мс, Δt_loop=150 мс)", "tab:blue", "--"),
    ("var_kp_10.csv", "Збільшений Kp=10.0 (T=50 мс, Δt_loop=150 мс)", "tab:purple", "-."),
    ("var_T_150ms.csv", "Період T=150 мс (Kp=5.0, Δt_loop=150 мс)", "tab:orange", ":"),
    ("var_delay_300ms.csv", "Затримка Δt_loop=300 мс (Kp=5.0, T=50 мс)", "tab:brown", "-"),
]

for fname, label, color, style in vars_list:
    t, x, v = load_csv(fname)
    ax1.plot(t, x, label=label, color=color, linestyle=style, linewidth=1.8)
    ax2.plot(t, v, label=label, color=color, linestyle=style, linewidth=1.8)

ax1.axhline(0.05, color="black", linestyle=":", linewidth=1.2, label="Ціль x_target = 0.05 рад")
ax1.set_ylabel("Положення x, рад")
ax1.set_title("Вплив параметрів Kp, T та Δt_loop на перерегулювання і автоколивання")
ax1.grid(True, alpha=0.4)
ax1.legend(loc="lower right", fontsize=8.5)

ax2.axhline(0.0, color="black", linestyle=":", linewidth=1.0)
ax2.set_xlabel("Час t, с")
ax2.set_ylabel("Швидкість v, рад/с")
ax2.grid(True, alpha=0.4)

plt.tight_layout()
plt.savefig(os.path.join(base_dir, "fig2_parametric_analysis.png"), dpi=200)
plt.close()
print("Графіки успішно збережено: fig1_main_comparison.png та fig2_parametric_analysis.png")
