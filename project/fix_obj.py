import sys

filepath = r'C:\Users\k024g\Lesson\2025A\CG2\CG2\project\resources\3dModels\wall\wall.obj'
try:
    with open(filepath, 'r') as f:
        lines = f.readlines()
except Exception as e:
    print('Failed to open:', e)
    sys.exit(1)

new_lines = []
for line in lines:
    if line.startswith('f '):
        parts = line.strip().split()
        if len(parts) >= 4:
            reversed_verts = parts[1:][::-1]
            new_lines.append('f ' + ' '.join(reversed_verts) + '\n')
        else:
            new_lines.append(line)
    elif line.startswith('vn '):
        parts = line.strip().split()
        if len(parts) == 4:
            new_lines.append(f'vn {-float(parts[1])} {-float(parts[2])} {-float(parts[3])}\n')
        else:
            new_lines.append(line)
    else:
        new_lines.append(line)

with open(filepath, 'w') as f:
    f.writelines(new_lines)
print('Done flipping wall.obj')
