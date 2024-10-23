from pathlib import Path
import pandas as pd

cpl_file_path = Path(__file__).parent.parent.parent.parent.parent / 'Hardware' / 'Project Outputs for Liv_Flo_Sign' / 'Liv_Flo_Sign_V1.0 CPL.xlsx'

# Load the CPL file
cpl = pd.read_excel(cpl_file_path) 

# We make only use of the following columns (header names): Designator, Mid X, Mid Y, Rotation
cplo = cpl[['Designator', 'Mid X', 'Mid Y', 'Rotation']]
cplo = cplo.rename(columns={'Designator': 'Ref', 'Mid X': 'PosX', 'Mid Y': 'PosY', 'Rotation': 'Rot'})

# We are only looking for designator that start with "P"
cplo = cplo[cplo['Ref'].str.startswith('P')]
cplo = cplo.reset_index(drop=True)

# Convert all text entries to float values, create a list of dictionaries containing all components
components = []
for index, row in cplo.iterrows():
    components.append({
        'Ref': row['Ref'],
        'PosX': float(row['PosX']),
        'PosY': float(row['PosY']),
        'Rot': float(row['Rot'])
    })

# Split list into two lists: LED_Matrix (0 ... 279), LED_Sign (280 ... end)
LED_Matrix = components[:280]
LED_Sign = components[280:]

def get_components():
    return LED_Matrix, LED_Sign
