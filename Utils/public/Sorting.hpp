
#pragma once

#include "Utils.hpp"

namespace tp {

	template <typename Type>
	inline bool compare(const Type& val1, const Type& val2) {
		return val1 > val2;
	}

	struct SortMerge {

		template <typename Type>
		static void sort(Type* buff, int length, bool (*grater)(const Type& obj1, const Type& obj2) = &compare) {
			mergeSort(buff, 0, length - 1, grater);
		}

		private:

		template <typename Type>
		static void merge(Type* buff, int left, int middle, int right, bool (*grater)(const Type& obj1, const Type& obj2)) {
			int n1 = middle - left + 1;
			int n2 = right - middle;

			Type* Left = new Type[n1];
			Type* Right = new Type[n2];

			for (int i = 0; i < n1; i++) {
				Left[i] = buff[left + i];
			}

			for (int j = 0; j < n2; j++) {
				Right[j] = buff[middle + 1 + j];
			}

			int i = 0;
			int j = 0;
			int k = left;

			while (i < n1 && j < n2) {
				if (!(grater(Left[i], Right[j]))) {
					buff[k] = Left[i];
					i++;
				} else {
					buff[k] = Right[j];
					j++;
				}
				k++;
			}

			while (i < n1) {
				buff[k] = Left[i];
				i++;
				k++;
			}

			while (j < n2) {
				buff[k] = Right[j];
				j++;
				k++;
			}

			delete[] Left;
			delete[] Right;
		}

		template <typename Type>
		static void mergeSort(Type* buff, int left, int right, bool (*grater)(const Type& obj1, const Type& obj2)) {

			if (left >= right) {
				return;
			}

			int middle = left + (right - left) / 2;

			mergeSort(buff, left, middle, grater);
			mergeSort(buff, middle + 1, right, grater);
			merge(buff, left, middle, right, grater);
		}

	};

	struct SortInsert {
		template <typename Type>
		static void sort(Type* buff, int length, bool (*grater)(const Type& obj1, const Type& obj2) = &compare) {
			for (int i = 0; i < length; i++) {
				for (int j = i + 1; j < length; j++) {
					if (grater(*buff[i], *buff[j])) {
						swap(buff[i], buff[j]);
					}
				}
			}
		}
	};
}