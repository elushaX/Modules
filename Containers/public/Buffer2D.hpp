
#include "Buffer.hpp"

namespace tp {

	template< typename tType, ualni tSizeX, ualni tSizeY >
	using ConstSizeBuffer2D = ConstSizeBuffer<ConstSizeBuffer<tType, tSizeX>, tSizeY>;

	template <typename tType>
	class Buffer2D {
		typedef SelCopyArg<tType> tTypeArg;
		typedef Pair<uhalni, uhalni> Size;

		Size mSize;
		tType* mBuff = nullptr;

	public:

		Buffer2D() = default;

		~Buffer2D() {
			delete mBuff;
		}

		explicit Buffer2D(Pair<ualni, ualni> aSize) {
			reserve(aSize);
		}

		[[nodiscard]] Pair<ualni, ualni> size() const {
			return { mSize.x, mSize.y };
		}

		tType* getBuff() const {
			return mBuff;
		}

		inline tType& get(uhalni x, uhalni y) {
			DEBUG_ASSERT(x < mSize.x && y < mSize.y && x >= 0 && y >= 0)
			return *(mBuff + mSize.x * y + x);
		}

		inline const tType& get(uhalni x, uhalni y) const {
			DEBUG_ASSERT(x < mSize.x && y < mSize.y && x >= 0 && y >= 0)
			return *(mBuff + mSize.x * y + x);
		}

		inline void set(uhalni x, uhalni y, tTypeArg value) {
			DEBUG_ASSERT(x < mSize.x && y < mSize.y && x >= 0 && y >= 0)
			*(mBuff + mSize.x * y + x) = value;
		}

		void reserve(Size newSize) {
			if (mSize.x != newSize.x || mSize.y != newSize.y) {
				delete mBuff;
				mBuff = new tType[newSize.x * newSize.y];
				mSize = newSize;
			}
		}

		void assign(tType value) {
			uhalni len = mSize.x * mSize.y;
			for (uhalni i = 0; i < len; i++) {
				mBuff[i] = value;
			}
		}
	};
}