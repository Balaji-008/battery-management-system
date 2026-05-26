def calculate_calibration():
    print("\n=== BMS V2 CALIBRATION TOOL ===")
    print("Use this to fix the 5% voltage difference on the LPC2148.\n")
    
    try:
        # Tap 1
        print("--- CELL 1 (Tap 1) ---")
        lcd_t1 = float(input("What does the LCD show for T1? (e.g. 3.90): "))
        true_t1 = float(input("What does the Multimeter show for Cell 1? (e.g. 4.11): "))
        old_cal1 = 1.0371
        new_cal1 = (true_t1 / lcd_t1) * old_cal1

        # Tap 2
        print("\n--- CELL 2 (Tap 2) ---")
        lcd_t2 = float(input("What does the LCD show for T2? (e.g. 8.10): "))
        true_t2 = float(input("What does the Multimeter show for Tap 2 (Cell1+Cell2)? (e.g. 8.23): "))
        old_cal2 = 1.0541
        new_cal2 = (true_t2 / lcd_t2) * old_cal2

        # Tap 3
        print("\n--- CELL 3 (Tap 3) ---")
        lcd_t3 = float(input("What does the LCD show for T3? (e.g. 12.10): "))
        true_t3 = float(input("What does the Multimeter show for Tap 3 (Pack Voltage)? (e.g. 12.35): "))
        old_cal3 = 0.9957
        new_cal3 = (true_t3 / lcd_t3) * old_cal3

        print("\n==================================================")
        print("✅ CALIBRATION COMPLETE! COPY AND PASTE THIS INTO main.c:")
        print("==================================================")
        print(f"    float CAL_1 = {new_cal1:.4f}; ")
        print(f"    float CAL_2 = {new_cal2:.4f}; ")
        print(f"    float CAL_3 = {new_cal3:.4f}; ")
        print("==================================================\n")

    except ValueError:
        print("Error: Please enter numbers only!")

if __name__ == "__main__":
    calculate_calibration()
