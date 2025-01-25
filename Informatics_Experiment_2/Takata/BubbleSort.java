public class BubbleSort {
    public static void main(String[] args) {
        int[] data ={5, 1, 5, 2, 5, 3, 5, 4, 5};

        bubbleSort(data);

        System.out.println("0");
    }

    public static void bubbleSort(int[] array) {
        int n = array.length;
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - 1 - i; j++) {
                if (array[j] > array[j + 1]) {
                    // Swap array[j] and array[j+1]
                    int temp = array[j];
                    array[j] = array[j + 1];
                    array[j + 1] = temp;
                }
            }
        }
    }
}
