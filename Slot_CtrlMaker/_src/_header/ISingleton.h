#pragma once
/* This header file was written based on these web pages.
1. http://d.hatena.ne.jp/buti2112/touch/20110305/1299354753
2. http://torokeru.tv/mameblog/?p=309
*/
template<class T> 
class ISingleton{
// [act]シングルトンクラスの基底クラス
	T& operator=(const ISingleton<T>&);
	ISingleton(const ISingleton<T>&);
protected:
	ISingleton(){};
	virtual ~ISingleton(){};
public:
	static T& GetInstance(){
		static T ins;
		return ins;
	}
};
/* 使い方:
	1.	このインターフェイスを使用するクラスで、ISingletonをパブリック継承する。
		ただしテンプレート指定はインターフェイスを使用するクラスの名前とする。
	2.	このインターフェイスを使用するクラスにおいて、コンストラクタとデストラクタは
		プライベートメンバとして宣言する。
	3.	このインターフェイスを使用するクラスにおいて、
			friend ISingleton<ClassName>
		を記述し、このクラス ISingleton からインターフェイスを使用するクラスの
		private / protected メンバにアクセスできるようにする。
	4.	1-3の操作によってインターフェイスを使用するクラスのコンストラクタは
		この ISingleton クラスからのみ呼べるようになり、外部からは一切宣言できなくなる。
	
	5.	呼び出しは以下のコードで行う。初回のみインスタンス生成は自動で行われる。
			ClassName& ValName = ClassName::GetInstance(); 
		※ 以下のような通常のインスタンス生成はコンパイルエラーとなる
			ClassName hoge;
	6.	関数呼び出しは次のコードで行う(参照型なので.アクセス)
			ValName.FunctionName(p1,p2,...);
			
実装例:
	// 宣言部 //
	class CHoge : public ISingleton<CHoge>{	// ISingletonをパブリック継承		使い方 1.
		friend ISingleton<Hoge>;			// ISingletonクラスをフレンド宣言	使い方 3.
	private:
		CHoge();							// コンストラクタをprivate宣言		使い方 2.
		~CHoge();							// 　デストラクタをprivate宣言		使い方 2.
		int m_memberVariable;				// 以下独自のメンバ変数など
		
	public:
		int Method1();						// 以下独自のメソッドなど
	}

	// 呼び出し部 //
	CHoge& instance = CHoge::GetInstance();	// インスタンスの生成方法			使い方 5.
	int a = instance.Method1();				// クラスメソッドの使用				使い方 6.
*/
